#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "asm.h"
#include "processor.h"
#include "sys_state.h"
#include "lwp_stack.h"
#include "lwp_threads.h"
#include "lwp_threadq.h"
#include "lwp_watchdog.h"

#define LWP_MAXPRIORITIES		256

/* new one */
frame_context core_context;

Thread_Control *_thr_main = NULL;
Thread_Control *_thr_idle = NULL;

Thread_Control *_thr_executing = NULL;
Thread_Control *_thr_heir = NULL;
Thread_Control *_thr_allocated_fp = NULL;

vu32 _context_switch_want;
vu32 _thread_dispatch_disable_level;

Watchdog_Control _lwp_wd_timeslice;
u32 _lwp_ticks_per_timeslice = 0;
void **__lwp_thr_libc_reent = NULL;
Chain_Control _lwp_thr_ready[LWP_MAXPRIORITIES];

static void (*_lwp_exitfunc)(void);

extern void _CPU_Context_switch(void *,void *);
extern void _cpu_context_switch_ex(void *,void *);
extern void _CPU_Context_save(void *);
extern void _CPU_Context_restore(void *);
extern void _CPU_Context_save_fp_context(void *);
extern void _CPU_Context_restore_fp_context(void *);

extern int libc_create_hook(Thread_Control *,Thread_Control *);
extern int libc_start_hook(Thread_Control *,Thread_Control *);
extern int libc_delete_hook(Thread_Control *, Thread_Control *);

extern void kprintf(const char *str, ...);

#ifdef _LWPTHREADS_DEBUG
static void __lwp_dumpcontext(frame_context *ctx)
{
	kprintf("GPR00 %08x GPR08 %08x GPR16 %08x GPR24 %08x\n",ctx->GPR[0], ctx->GPR[8], ctx->GPR[16], ctx->GPR[24]);
	kprintf("GPR01 %08x GPR09 %08x GPR17 %08x GPR25 %08x\n",ctx->GPR[1], ctx->GPR[9], ctx->GPR[17], ctx->GPR[25]);
	kprintf("GPR02 %08x GPR10 %08x GPR18 %08x GPR26 %08x\n",ctx->GPR[2], ctx->GPR[10], ctx->GPR[18], ctx->GPR[26]);
	kprintf("GPR03 %08x GPR11 %08x GPR19 %08x GPR27 %08x\n",ctx->GPR[3], ctx->GPR[11], ctx->GPR[19], ctx->GPR[27]);
	kprintf("GPR04 %08x GPR12 %08x GPR20 %08x GPR28 %08x\n",ctx->GPR[4], ctx->GPR[12], ctx->GPR[20], ctx->GPR[28]);
	kprintf("GPR05 %08x GPR13 %08x GPR21 %08x GPR29 %08x\n",ctx->GPR[5], ctx->GPR[13], ctx->GPR[21], ctx->GPR[29]);
	kprintf("GPR06 %08x GPR14 %08x GPR22 %08x GPR30 %08x\n",ctx->GPR[6], ctx->GPR[14], ctx->GPR[22], ctx->GPR[30]);
	kprintf("GPR07 %08x GPR15 %08x GPR23 %08x GPR31 %08x\n",ctx->GPR[7], ctx->GPR[15], ctx->GPR[23], ctx->GPR[31]);
	kprintf("LR %08x SRR0 %08x SRR1 %08x MSR %08x\n\n", ctx->LR, ctx->SRR0, ctx->SRR1,ctx->MSR);
}

void __lwp_dumpcontext_fp(Thread_Control *thrA,Thread_Control *thrB)
{
	kprintf("_cpu_contextfp_dump(%p,%p)\n",thrA,thrB);
}
#endif

u32 _ISR_Is_in_progress()
{
	register u32 isr_nest_level;
	isr_nest_level = mfspr(272);
	return isr_nest_level;
}

static inline void _CPU_ISR_Set_level(u32 level)
{
	register u32 msr;
	_CPU_MSR_GET(msr);
	if(!(level&CPU_MODES_INTERRUPT_MASK))
		msr |= MSR_EE;
	else
		msr &= ~MSR_EE;
	_CPU_MSR_SET(msr);
}

static inline u32 _CPU_ISR_Get_level()
{
	register u32 msr;
	_CPU_MSR_GET(msr);
	if(msr&MSR_EE) return 0;
	else return 1;
}

void _Thread_Delay_ended(void *arg)
{
	Thread_Control *thethread = (Thread_Control*)arg;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Delay_ended(%p)\n",thethread);
#endif
	if(!thethread) return;
	
	_Thread_Disable_dispatch();
	_Thread_Unblock(thethread);
	_Thread_Unnest_dispatch();
}

void _Thread_Tickle_timeslice(void *arg)
{
	s64 ticks;
	Thread_Control *exec;

	exec = _thr_executing;
	ticks = millisecs_to_ticks(1);
	
	_Thread_Disable_dispatch();

	if(!exec->is_preemptible) {
		_Watchdog_Insert_ticks(&_lwp_wd_timeslice,ticks);
		_Thread_Unnest_dispatch();
		return;
	}
	if(!_States_Is_ready(exec->cur_state)) {
		_Watchdog_Insert_ticks(&_lwp_wd_timeslice,ticks);
		_Thread_Unnest_dispatch();
		return;
	}

	switch(exec->budget_algo) {
		case LWP_CPU_BUDGET_ALGO_NONE:
			break;
		case LWP_CPU_BUDGET_ALGO_TIMESLICE:
			if((--exec->cpu_time_budget)==0) {
				_Thread_Reset_timeslice();
				exec->cpu_time_budget = _lwp_ticks_per_timeslice;
			}
			break;
	}

	_Watchdog_Insert_ticks(&_lwp_wd_timeslice,ticks);
	_Thread_Unnest_dispatch();
}

void __thread_dispatch_fp()
{
	u32 level;
	Thread_Control *exec;

	_CPU_ISR_Disable(level);
	exec = _thr_executing;
#ifdef _LWPTHREADS_DEBUG
	__lwp_dumpcontext_fp(exec,_thr_allocated_fp);
#endif
	if(!_Thread_Is_allocated_fp(exec)) {
		if(_thr_allocated_fp) _CPU_Context_save_fp_context(&_thr_allocated_fp->context);
		_CPU_Context_restore_fp_context(&exec->context);
		_thr_allocated_fp = exec;
	}
	_CPU_ISR_Restore(level);
}

void _Thread_Dispatch()
{
	u32 level;
	Thread_Control *exec,*heir;

	_CPU_ISR_Disable(level);
	exec = _thr_executing;
	while(_context_switch_want==TRUE) {
		heir = _thr_heir;
		_thread_dispatch_disable_level = 1;
		_context_switch_want = FALSE;
		_thr_executing = heir;
		_CPU_ISR_Restore(level);

		if(__lwp_thr_libc_reent) {
			exec->libc_reent = *__lwp_thr_libc_reent;
			*__lwp_thr_libc_reent = heir->libc_reent;
		}
#ifdef _DEBUG
		_cpu_context_switch_ex((void*)&exec->context,(void*)&heir->context);
#else
		_CPU_Context_switch((void*)&exec->context,(void*)&heir->context);
#endif
		exec = _thr_executing;
		_CPU_ISR_Disable(level);
	}
	_thread_dispatch_disable_level = 0;
	_CPU_ISR_Restore(level);
}

static void _Thread_Handler()
{
	u32 level;
	Thread_Control *exec;

	exec = _thr_executing;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Handler(%p,%d)\n",exec,_thread_dispatch_disable_level);
#endif
	level = exec->isr_level;
	_CPU_ISR_Set_level(level);
	_Thread_Enable_dispatch();
	exec->wait.return_argument = exec->entry(exec->arg);

	pthread_exit(exec->wait.return_argument);
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Handler(%p): thread returned(%p)\n",exec,exec->wait.return_argument);
#endif
}

void _Thread_Rotate_Ready_Queue(u32 prio)
{
	u32 level;
	Thread_Control *exec;
	Chain_Control *ready;
	Chain_Node *node;

	ready = &_lwp_thr_ready[prio];
	exec = _thr_executing;
	
	if(ready==exec->ready) {
		_Thread_Yield_processor();
		return;
	}

	_CPU_ISR_Disable(level);
	if(!_Chain_Is_empty(ready) && !_Chain_Has_only_one_node(ready)) {
		node = _Chain_Get_first_unprotected(ready);
		_Chain_Append_unprotected(ready,node);
	}
	_CPU_ISR_Flash(level);
	
	if(_thr_heir->ready==ready)
		_thr_heir = (Thread_Control*)ready->first;

	if(exec!=_thr_heir)
		_context_switch_want = TRUE;

#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Rotate_Ready_Queue(%d,%p,%p)\n",prio,exec,_thr_heir);
#endif
	_CPU_ISR_Restore(level);
}

void _Thread_Yield_processor()
{
	u32 level;
	Thread_Control *exec;
	Chain_Control *ready;
	
	exec = _thr_executing;
	ready = exec->ready;
	
	_CPU_ISR_Disable(level);
	if(!_Chain_Has_only_one_node(ready)) {
		_Chain_Extract_unprotected(&exec->object.Node);
		_Chain_Append_unprotected(ready,&exec->object.Node);
		_CPU_ISR_Flash(level);
		if(_Thread_Is_heir(exec))
			_thr_heir = (Thread_Control*)ready->first;
		_context_switch_want = TRUE;
	} else if(!_Thread_Is_heir(exec))
		_context_switch_want = TRUE;
	_CPU_ISR_Restore(level);
}

void _Thread_Reset_timeslice()
{
	u32 level;
	Thread_Control *exec;
	Chain_Control *ready;

	exec = _thr_executing;
	ready = exec->ready;

	_CPU_ISR_Disable(level);
	if(_Chain_Has_only_one_node(ready)) {
		_CPU_ISR_Restore(level);
		return;
	}

	_Chain_Extract_unprotected(&exec->object.Node);
	_Chain_Append_unprotected(ready,&exec->object.Node);

	_CPU_ISR_Flash(level);

	if(_Thread_Is_heir(exec))
		_thr_heir = (Thread_Control*)ready->first;

	_context_switch_want = TRUE;
	_CPU_ISR_Restore(level);
}

void _Thread_Set_state(Thread_Control *thethread,u32 state)
{
	u32 level;
	Chain_Control *ready;

	ready = thethread->ready;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Set_state(%d,%p,%p,%08x)\n",_context_switch_want,_thr_heir,thethread,thethread->cur_state);
#endif
	_CPU_ISR_Disable(level);
	if(!_States_Is_ready(thethread->cur_state)) {
		thethread->cur_state = _States_Clear(thethread->cur_state,state);
		_CPU_ISR_Restore(level);
		return;
	}

	thethread->cur_state = state;
	if(_Chain_Has_only_one_node(ready)) {
		_Chain_Initialize_empty(ready);
		_Priority_Remove_from_bit_map(&thethread->priomap);
	} else 
		_Chain_Extract_unprotected(&thethread->object.Node);
	_CPU_ISR_Flash(level);

	if(_Thread_Is_heir(thethread))
		_Thread_Calculate_heir();
	if(_Thread_Is_executing(thethread))
		_context_switch_want = TRUE;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Set_state(%d,%p,%p,%08x)\n",_context_switch_want,_thr_heir,thethread,thethread->cur_state);
#endif
	_CPU_ISR_Restore(level);
}

void _Thread_Clear_state(Thread_Control *thethread,u32 state)
{
	u32 level,cur_state;
	
	_CPU_ISR_Disable(level);

	cur_state = thethread->cur_state;
	if(cur_state&state) {
		cur_state = thethread->cur_state = _States_Clear(cur_state,state);
		if(_States_Is_ready(cur_state)) {
			_Priority_Add_to_bit_map(&thethread->priomap);
			_Chain_Append_unprotected(thethread->ready,&thethread->object.Node);
			_CPU_ISR_Flash(level);
			
			if(thethread->cur_prio<_thr_heir->cur_prio) {
				_thr_heir = thethread;
				if(_thr_executing->is_preemptible
					|| thethread->cur_prio==0)
				_context_switch_want = TRUE;
			}
		}
	}

	_CPU_ISR_Restore(level);
}

u32 _Thread_Evaluate_mode()
{
	Thread_Control *exec;
	
	exec = _thr_executing;
	if(!_States_Is_ready(exec->cur_state)
		|| (!_Thread_Is_heir(exec) && exec->is_preemptible)){
		_context_switch_want = TRUE;
		return TRUE;
	}
	return FALSE;
}

void _Thread_Change_priority(Thread_Control *thethread,u32 prio,u32 prependit)
{
	u32 level;
	
	_Thread_Set_transient(thethread);
	
	if(thethread->cur_prio!=prio) 
		_Thread_Set_priority(thethread,prio);

	_CPU_ISR_Disable(level);

	thethread->cur_state = _States_Clear(thethread->cur_state,LWP_STATES_TRANSIENT);
	if(!_States_Is_ready(thethread->cur_state)) {
		_CPU_ISR_Restore(level);
		return;
	}

	_Priority_Add_to_bit_map(&thethread->priomap);
	if(prependit)
		_Chain_Prepend_unprotected(thethread->ready,&thethread->object.Node);
	else
		_Chain_Append_unprotected(thethread->ready,&thethread->object.Node);

	_CPU_ISR_Flash(level);

	_Thread_Calculate_heir();
	
	if(!(_thr_executing==_thr_heir)
		&& _thr_executing->is_preemptible)
		_context_switch_want = TRUE;

	_CPU_ISR_Restore(level);
}

void _Thread_Set_priority(Thread_Control *thethread,u32 prio)
{
	thethread->cur_prio = prio;
	thethread->ready = &_lwp_thr_ready[prio];
	_Priority_Initialize_information(&thethread->priomap,prio);
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Set_priority(%p,%d,%p)\n",thethread,prio,thethread->ready);
#endif
}

void _Thread_Suspend(Thread_Control *thethread)
{
	u32 level;
	Chain_Control *ready;

	ready = thethread->ready;
	
	_CPU_ISR_Disable(level);
	thethread->suspendcnt++;
	if(!_States_Is_ready(thethread->cur_state)) {
		thethread->cur_state = _States_Set(thethread->cur_state,LWP_STATES_SUSPENDED);
		_CPU_ISR_Restore(level);
		return;
	}
	
	thethread->cur_state = LWP_STATES_SUSPENDED;
	if(_Chain_Has_only_one_node(ready)) {
		_Chain_Initialize_empty(ready);
		_Priority_Remove_from_bit_map(&thethread->priomap);
	} else {
		_Chain_Extract_unprotected(&thethread->object.Node);
	}
	_CPU_ISR_Flash(level);
	
	if(_Thread_Is_heir(thethread))
		_Thread_Calculate_heir();
	
	if(_Thread_Is_executing(thethread))
		_context_switch_want = TRUE;

	_CPU_ISR_Restore(level);
}

void _Thread_Set_transient(Thread_Control *thethread)
{
	u32 level,oldstates;
	Chain_Control *ready;

	ready = thethread->ready;

	_CPU_ISR_Disable(level);
	
	oldstates = thethread->cur_state;
	thethread->cur_state = _States_Set(oldstates,LWP_STATES_TRANSIENT);

	if(_States_Is_ready(oldstates)) {
		if(_Chain_Has_only_one_node(ready)) {
			_Chain_Initialize_empty(ready);
			_Priority_Remove_from_bit_map(&thethread->priomap);
		} else {
			_Chain_Extract_unprotected(&thethread->object.Node);
		}
	}

	_CPU_ISR_Restore(level);
}

void _Thread_Resume(Thread_Control *thethread,u32 force)
{
	u32 level,state;
	
	_CPU_ISR_Disable(level);

	if(force==TRUE)
		thethread->suspendcnt = 0;
	else
		thethread->suspendcnt--;
	
	if(thethread->suspendcnt>0) {
		_CPU_ISR_Restore(level);
		return;
	}

	state = thethread->cur_state;
	if(state&LWP_STATES_SUSPENDED) {
		state = thethread->cur_state = _States_Clear(thethread->cur_state,LWP_STATES_SUSPENDED);
		if(_States_Is_ready(state)) {
			_Priority_Add_to_bit_map(&thethread->priomap);
			_Chain_Append_unprotected(thethread->ready,&thethread->object.Node);
			_CPU_ISR_Flash(level);
			if(thethread->cur_prio<_thr_heir->cur_prio) {
				_thr_heir = thethread;
				if(_thr_executing->is_preemptible
					|| thethread->cur_prio==0)
				_context_switch_want = TRUE;
			}
		}
	}
	_CPU_ISR_Restore(level);
}

void _Thread_Load_environment(Thread_Control *thethread)
{
	u32 stackbase,sp,size;
	u32 r2,r13,msr_value;
	
	thethread->context.FPSCR = 0x000000f8;

	stackbase = (u32)thethread->stack;
	size = thethread->stack_size;

	// tag both bottom & head of stack
	*((u32*)stackbase) = 0xDEADBABE;
	sp = stackbase+size-CPU_MINIMUM_STACK_FRAME_SIZE;
	sp &= ~(CPU_STACK_ALIGNMENT-1);
	*((u32*)sp) = 0;
	
	thethread->context.GPR[1] = sp;
	
	msr_value = (MSR_ME|MSR_IR|MSR_DR|MSR_RI);
	if(!(thethread->isr_level&CPU_MODES_INTERRUPT_MASK))
		msr_value |= MSR_EE;
	
	thethread->context.MSR = msr_value;
	thethread->context.LR = (u32)_Thread_Handler;

	__asm__ __volatile__ ("mr %0,2; mr %1,13" : "=r" ((r2)), "=r" ((r13)));
	thethread->context.GPR[2] = r2;
	thethread->context.GPR[13] = r13;

#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Load_environment(%p,%p,%d,%p)\n",thethread,(void*)stackbase,size,(void*)sp);
#endif

}

void _Thread_Ready(Thread_Control *thethread)
{
	u32 level;
	Thread_Control *heir;

	_CPU_ISR_Disable(level);
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Ready(%p)\n",thethread);
#endif
	thethread->cur_state = LWP_STATES_READY;
	_Priority_Add_to_bit_map(&thethread->priomap);
	_Chain_Append_unprotected(thethread->ready,&thethread->object.Node);
	_CPU_ISR_Flash(level);

	_Thread_Calculate_heir();
	heir = _thr_heir;
	if(!(_Thread_Is_executing(heir)) && _thr_executing->is_preemptible)
		_context_switch_want = TRUE;
	
	_CPU_ISR_Restore(level);
}

u32 _Thread_Initialize(Thread_Control *thethread,void *stack_area,u32 stack_size,u32 prio,u32 isr_level,bool is_preemtible)
{
	u32 act_stack_size = 0;

#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Initialize(%p,%p,%d,%d,%d)\n",thethread,stack_area,stack_size,prio,isr_level);
#endif

	if(!stack_area) {
		if(!_Stack_Is_enough(stack_size))
			act_stack_size = CPU_MINIMUM_STACK_SIZE;
		else
			act_stack_size = stack_size;

		act_stack_size = _Thread_Stack_Allocate(thethread,act_stack_size);
		if(!act_stack_size) return 0;

		thethread->stack_allocated = TRUE;
	} else {
		thethread->stack = stack_area;
		act_stack_size = stack_size;
		thethread->stack_allocated = FALSE;
	}
	thethread->stack_size = act_stack_size;

	_Thread_queue_Initialize(&thethread->join_list,LWP_THREADQ_MODEFIFO,LWP_STATES_WAITING_FOR_JOINATEXIT,0);

	memset(&thethread->context,0,sizeof(thethread->context));
	memset(&thethread->wait,0,sizeof(thethread->wait));

	thethread->budget_algo = (prio<128 ? LWP_CPU_BUDGET_ALGO_NONE : LWP_CPU_BUDGET_ALGO_TIMESLICE);
	thethread->is_preemptible = is_preemtible;
	thethread->isr_level = isr_level;
	thethread->real_prio = prio;
	thethread->cur_state = LWP_STATES_DORMANT;
	thethread->cpu_time_budget = _lwp_ticks_per_timeslice;
	thethread->suspendcnt = 0;
	thethread->res_cnt = 0;
	_Thread_Set_priority(thethread,prio);

	libc_create_hook(_thr_executing,thethread);

	return 1;
}

void _Thread_Close(Thread_Control *thethread)
{
	u32 level;
	void **value_ptr;
	Thread_Control *p;

	_Thread_Set_state(thethread,LWP_STATES_TRANSIENT);
	
	if(!_Thread_queue_Extract_with_proxy(thethread)) {
		if(_Watchdog_Is_active(&thethread->timer))
			_Watchdog_Remove_ticks(&thethread->timer);
	}
	
	_CPU_ISR_Disable(level);
	value_ptr = (void**)thethread->wait.return_argument;
	while((p=_Thread_queue_Dequeue(&thethread->join_list))!=NULL) {
		*(void**)p->wait.return_argument = value_ptr;
	}
	thethread->cpu_time_budget = 0;
	thethread->budget_algo = LWP_CPU_BUDGET_ALGO_NONE;
	_CPU_ISR_Restore(level);

	libc_delete_hook(_thr_executing,thethread);

	if(_Thread_Is_allocated_fp(thethread))
		_Thread_Deallocate_fp();

	_Thread_Stack_Free(thethread);

	_Objects_Close(thethread->object.information,&thethread->object);
	_Objects_Free(thethread->object.information,&thethread->object);
}

void __lwp_thread_closeall()
{
	u32 i,level;
	Chain_Control *header;
	Thread_Control *ptr,*next;
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_closeall(enter)\n");
#endif
	_CPU_ISR_Disable(level);
	for(i=0;i<LWP_MAXPRIORITIES;i++) {
		header = &_lwp_thr_ready[i];
		ptr = (Thread_Control*)header->first;
		while(ptr!=(Thread_Control*)_Chain_Tail(&_lwp_thr_ready[i])) {
			next = (Thread_Control*)ptr->object.Node.next;
			if(ptr!=_thr_executing)
				_Thread_Close(ptr);

			ptr = next;
		}
	}
	_CPU_ISR_Restore(level);
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_closeall(leave)\n");
#endif
}

void pthread_exit(void *value_ptr)
{
	_Thread_Disable_dispatch();
	_thr_executing->wait.return_argument = (u32*)value_ptr;
	_Thread_Close(_thr_executing);
	_Thread_Enable_dispatch();
}

u32 _Thread_Start(Thread_Control *thethread,void* (*entry)(void*),void *arg)
{
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Start(%p,%p,%p,%d)\n",thethread,entry,arg,thethread->cur_state);
#endif
	if(_States_Is_dormant(thethread->cur_state)) {
		thethread->entry = entry;
		thethread->arg = arg;
		_Thread_Load_environment(thethread);
		_Thread_Ready(thethread);
		libc_start_hook(_thr_executing,thethread);
		return 1;
	}
	return 0;
}

void _Thread_Start_multitasking()
{
	_lwp_exitfunc = NULL;

	_System_state_Set(SYS_STATE_BEGIN_MT);
	_System_state_Set(SYS_STATE_UP);

	_context_switch_want = FALSE;
	_thr_executing = _thr_heir;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Start_multitasking(%p,%p)\n",_thr_executing,_thr_heir);
#endif
	_Watchdog_Insert_ticks(&_lwp_wd_timeslice,millisecs_to_ticks(1));
	_CPU_Context_switch((void*)&core_context,(void*)&_thr_heir->context);

	if(_lwp_exitfunc) _lwp_exitfunc();
}

void _Thread_Stop_multitasking(void (*exitfunc)())
{
	_lwp_exitfunc = exitfunc;
	if(_System_state_Get()!=SYS_STATE_SHUTDOWN) {
		_Watchdog_Remove_ticks(&_lwp_wd_timeslice);
		_System_state_Set(SYS_STATE_SHUTDOWN);
		_CPU_Context_switch((void*)&_thr_executing->context,(void*)&core_context);
	}
}

void _Thread_Handler_initialization()
{
	u32 index;

#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Handler_initialization()\n\n");
#endif
	_Thread_Dispatch_initialization();
	_Watchdog_Initialize(&_lwp_wd_timeslice,_Thread_Tickle_timeslice,LWP_TIMESLICE_TIMER_ID,NULL);
	
	_context_switch_want = FALSE;
	_thr_executing = NULL;
	_thr_heir = NULL;
	_thr_allocated_fp = NULL;
	_lwp_ticks_per_timeslice = 10;

	memset(&core_context,0,sizeof(core_context));

	for(index=0;index<=LWP_PRIO_MAX;index++)
		_Chain_Initialize_empty(&_lwp_thr_ready[index]);
	
	_System_state_Set(SYS_STATE_BEFORE_MT);
}

