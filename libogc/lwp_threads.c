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
Context_Control _Thread_BSP_context;

Thread_Control *_thr_main = NULL;
Thread_Control *_Thread_Idle = NULL;

Thread_Control *_Thread_Executing = NULL;
Thread_Control *_Thread_Heir = NULL;
Thread_Control *_Thread_Allocated_fp = NULL;

vu32 _Context_Switch_necessary;
vu32 _Thread_Dispatch_disable_level;

Watchdog_Control _lwp_wd_timeslice;
u32 _Thread_Ticks_per_timeslice = 0;
void **_Thread_libc_reent = NULL;
Chain_Control _Thread_Ready_chain[LWP_MAXPRIORITIES];

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
	register u32 isr_nesting_level;
	isr_nesting_level = mfspr(272);
	return isr_nesting_level;
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
	Thread_Control *the_thread = (Thread_Control*)arg;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Delay_ended(%p)\n",the_thread);
#endif
	if(!the_thread) return;
	
	_Thread_Disable_dispatch();
	_Thread_Unblock(the_thread);
	_Thread_Unnest_dispatch();
}

void _Thread_Tickle_timeslice(void *arg)
{
	s64 ticks;
	Thread_Control *executing;

	executing = _Thread_Executing;
	ticks = millisecs_to_ticks(1);
	
	_Thread_Disable_dispatch();

	if(!executing->is_preemptible) {
		_Watchdog_Insert_ticks(&_lwp_wd_timeslice,ticks);
		_Thread_Unnest_dispatch();
		return;
	}
	if(!_States_Is_ready(executing->current_state)) {
		_Watchdog_Insert_ticks(&_lwp_wd_timeslice,ticks);
		_Thread_Unnest_dispatch();
		return;
	}

	switch(executing->budget_algorithm) {
		case THREAD_CPU_BUDGET_ALGORITHM_NONE:
			break;
		case THREAD_CPU_BUDGET_ALGORITHM_RESET_TIMESLICE:
			if((--executing->cpu_time_budget)==0) {
				_Thread_Reset_timeslice();
				executing->cpu_time_budget = _Thread_Ticks_per_timeslice;
			}
			break;
	}

	_Watchdog_Insert_ticks(&_lwp_wd_timeslice,ticks);
	_Thread_Unnest_dispatch();
}

void __thread_dispatch_fp()
{
	u32 level;
	Thread_Control *executing;

	_CPU_ISR_Disable(level);
	executing = _Thread_Executing;
#ifdef _LWPTHREADS_DEBUG
	__lwp_dumpcontext_fp(executing,_Thread_Allocated_fp);
#endif
	if(!_Thread_Is_allocated_fp(executing)) {
		if(_Thread_Allocated_fp) _CPU_Context_save_fp_context(&_Thread_Allocated_fp->Registers);
		_CPU_Context_restore_fp_context(&executing->Registers);
		_Thread_Allocated_fp = executing;
	}
	_CPU_ISR_Restore(level);
}

void _Thread_Dispatch()
{
	u32 level;
	Thread_Control *executing,*heir;

	_CPU_ISR_Disable(level);
	executing = _Thread_Executing;
	while(_Context_Switch_necessary==TRUE) {
		heir = _Thread_Heir;
		_Thread_Dispatch_disable_level = 1;
		_Context_Switch_necessary = FALSE;
		_Thread_Executing = heir;
		_CPU_ISR_Restore(level);

		if(_Thread_libc_reent) {
			executing->libc_reent = *_Thread_libc_reent;
			*_Thread_libc_reent = heir->libc_reent;
		}
#ifdef _DEBUG
		_cpu_context_switch_ex((void*)&executing->Registers,(void*)&heir->Registers);
#else
		_CPU_Context_switch((void*)&executing->Registers,(void*)&heir->Registers);
#endif
		executing = _Thread_Executing;
		_CPU_ISR_Disable(level);
	}
	_Thread_Dispatch_disable_level = 0;
	_CPU_ISR_Restore(level);
}

static void _Thread_Handler()
{
	u32 level;
	Thread_Control *executing;

	executing = _Thread_Executing;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Handler(%p,%d)\n",executing,_Thread_Dispatch_disable_level);
#endif
	level = executing->isr_level;
	_CPU_ISR_Set_level(level);
	_Thread_Enable_dispatch();
	executing->Wait.return_argument = executing->entry_point(executing->pointer_argument);

	pthread_exit(executing->Wait.return_argument);
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Handler(%p): thread returned(%p)\n",executing,executing->wait.return_argument);
#endif
}

void _Thread_Rotate_Ready_Queue(u32 priority)
{
	u32 level;
	Thread_Control *executing;
	Chain_Control *ready;
	Chain_Node *node;

	ready = &_Thread_Ready_chain[priority];
	executing = _Thread_Executing;
	
	if(ready==executing->ready) {
		_Thread_Yield_processor();
		return;
	}

	_CPU_ISR_Disable(level);
	if(!_Chain_Is_empty(ready) && !_Chain_Has_only_one_node(ready)) {
		node = _Chain_Get_first_unprotected(ready);
		_Chain_Append_unprotected(ready,node);
	}
	_CPU_ISR_Flash(level);
	
	if(_Thread_Heir->ready==ready)
		_Thread_Heir = (Thread_Control*)ready->first;

	if(executing!=_Thread_Heir)
		_Context_Switch_necessary = TRUE;

#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Rotate_Ready_Queue(%d,%p,%p)\n",priority,executing,_Thread_Heir);
#endif
	_CPU_ISR_Restore(level);
}

void _Thread_Yield_processor()
{
	u32 level;
	Thread_Control *executing;
	Chain_Control *ready;
	
	executing = _Thread_Executing;
	ready = executing->ready;
	
	_CPU_ISR_Disable(level);
	if(!_Chain_Has_only_one_node(ready)) {
		_Chain_Extract_unprotected(&executing->Object.Node);
		_Chain_Append_unprotected(ready,&executing->Object.Node);
		_CPU_ISR_Flash(level);
		if(_Thread_Is_heir(executing))
			_Thread_Heir = (Thread_Control*)ready->first;
		_Context_Switch_necessary = TRUE;
	} else if(!_Thread_Is_heir(executing))
		_Context_Switch_necessary = TRUE;
	_CPU_ISR_Restore(level);
}

void _Thread_Reset_timeslice()
{
	u32 level;
	Thread_Control *executing;
	Chain_Control *ready;

	executing = _Thread_Executing;
	ready = executing->ready;

	_CPU_ISR_Disable(level);
	if(_Chain_Has_only_one_node(ready)) {
		_CPU_ISR_Restore(level);
		return;
	}

	_Chain_Extract_unprotected(&executing->Object.Node);
	_Chain_Append_unprotected(ready,&executing->Object.Node);

	_CPU_ISR_Flash(level);

	if(_Thread_Is_heir(executing))
		_Thread_Heir = (Thread_Control*)ready->first;

	_Context_Switch_necessary = TRUE;
	_CPU_ISR_Restore(level);
}

void _Thread_Set_state(Thread_Control *the_thread,u32 state)
{
	u32 level;
	Chain_Control *ready;

	ready = the_thread->ready;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Set_state(%d,%p,%p,%08x)\n",_Context_Switch_necessary,_Thread_Heir,the_thread,the_thread->cur_state);
#endif
	_CPU_ISR_Disable(level);
	if(!_States_Is_ready(the_thread->current_state)) {
		the_thread->current_state = _States_Clear(the_thread->current_state,state);
		_CPU_ISR_Restore(level);
		return;
	}

	the_thread->current_state = state;
	if(_Chain_Has_only_one_node(ready)) {
		_Chain_Initialize_empty(ready);
		_Priority_Remove_from_bit_map(&the_thread->Priority_map);
	} else 
		_Chain_Extract_unprotected(&the_thread->Object.Node);
	_CPU_ISR_Flash(level);

	if(_Thread_Is_heir(the_thread))
		_Thread_Calculate_heir();
	if(_Thread_Is_executing(the_thread))
		_Context_Switch_necessary = TRUE;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Set_state(%d,%p,%p,%08x)\n",_Context_Switch_necessary,_Thread_Heir,the_thread,the_thread->cur_state);
#endif
	_CPU_ISR_Restore(level);
}

void _Thread_Clear_state(Thread_Control *the_thread,u32 state)
{
	u32 level,current_state;
	
	_CPU_ISR_Disable(level);

	current_state = the_thread->current_state;
	if(current_state&state) {
		current_state = the_thread->current_state = _States_Clear(current_state,state);
		if(_States_Is_ready(current_state)) {
			_Priority_Add_to_bit_map(&the_thread->Priority_map);
			_Chain_Append_unprotected(the_thread->ready,&the_thread->Object.Node);
			_CPU_ISR_Flash(level);
			
			if(the_thread->current_priority<_Thread_Heir->current_priority) {
				_Thread_Heir = the_thread;
				if(_Thread_Executing->is_preemptible
					|| the_thread->current_priority==0)
				_Context_Switch_necessary = TRUE;
			}
		}
	}

	_CPU_ISR_Restore(level);
}

u32 _Thread_Evaluate_mode()
{
	Thread_Control *executing;
	
	executing = _Thread_Executing;
	if(!_States_Is_ready(executing->current_state)
		|| (!_Thread_Is_heir(executing) && executing->is_preemptible)){
		_Context_Switch_necessary = TRUE;
		return TRUE;
	}
	return FALSE;
}

void _Thread_Change_priority(Thread_Control *the_thread,u32 new_priority,u32 prepend_it)
{
	u32 level;
	
	_Thread_Set_transient(the_thread);
	
	if(the_thread->current_priority!=new_priority) 
		_Thread_Set_priority(the_thread,new_priority);

	_CPU_ISR_Disable(level);

	the_thread->current_state = _States_Clear(the_thread->current_state,STATES_TRANSIENT);
	if(!_States_Is_ready(the_thread->current_state)) {
		_CPU_ISR_Restore(level);
		return;
	}

	_Priority_Add_to_bit_map(&the_thread->Priority_map);
	if(prepend_it)
		_Chain_Prepend_unprotected(the_thread->ready,&the_thread->Object.Node);
	else
		_Chain_Append_unprotected(the_thread->ready,&the_thread->Object.Node);

	_CPU_ISR_Flash(level);

	_Thread_Calculate_heir();
	
	if(!(_Thread_Executing==_Thread_Heir)
		&& _Thread_Executing->is_preemptible)
		_Context_Switch_necessary = TRUE;

	_CPU_ISR_Restore(level);
}

void _Thread_Set_priority(Thread_Control *the_thread,u32 new_priority)
{
	the_thread->current_priority = new_priority;
	the_thread->ready = &_Thread_Ready_chain[new_priority];
	_Priority_Initialize_information(&the_thread->Priority_map,new_priority);
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Set_priority(%p,%d,%p)\n",the_thread,new_priority,the_thread->ready);
#endif
}

void _Thread_Suspend(Thread_Control *the_thread)
{
	u32 level;
	Chain_Control *ready;

	ready = the_thread->ready;
	
	_CPU_ISR_Disable(level);
	the_thread->suspend_count++;
	if(!_States_Is_ready(the_thread->current_state)) {
		the_thread->current_state = _States_Set(the_thread->current_state,STATES_SUSPENDED);
		_CPU_ISR_Restore(level);
		return;
	}
	
	the_thread->current_state = STATES_SUSPENDED;
	if(_Chain_Has_only_one_node(ready)) {
		_Chain_Initialize_empty(ready);
		_Priority_Remove_from_bit_map(&the_thread->Priority_map);
	} else {
		_Chain_Extract_unprotected(&the_thread->Object.Node);
	}
	_CPU_ISR_Flash(level);
	
	if(_Thread_Is_heir(the_thread))
		_Thread_Calculate_heir();
	
	if(_Thread_Is_executing(the_thread))
		_Context_Switch_necessary = TRUE;

	_CPU_ISR_Restore(level);
}

void _Thread_Set_transient(Thread_Control *the_thread)
{
	u32 level,old_state;
	Chain_Control *ready;

	ready = the_thread->ready;

	_CPU_ISR_Disable(level);
	
	old_state = the_thread->current_state;
	the_thread->current_state = _States_Set(old_state,STATES_TRANSIENT);

	if(_States_Is_ready(old_state)) {
		if(_Chain_Has_only_one_node(ready)) {
			_Chain_Initialize_empty(ready);
			_Priority_Remove_from_bit_map(&the_thread->Priority_map);
		} else {
			_Chain_Extract_unprotected(&the_thread->Object.Node);
		}
	}

	_CPU_ISR_Restore(level);
}

void _Thread_Resume(Thread_Control *the_thread,u32 force)
{
	u32 level,current_state;
	
	_CPU_ISR_Disable(level);

	if(force==TRUE)
		the_thread->suspend_count = 0;
	else
		the_thread->suspend_count--;
	
	if(the_thread->suspend_count>0) {
		_CPU_ISR_Restore(level);
		return;
	}

	current_state = the_thread->current_state;
	if(current_state&STATES_SUSPENDED) {
		current_state = the_thread->current_state = _States_Clear(the_thread->current_state,STATES_SUSPENDED);
		if(_States_Is_ready(current_state)) {
			_Priority_Add_to_bit_map(&the_thread->Priority_map);
			_Chain_Append_unprotected(the_thread->ready,&the_thread->Object.Node);
			_CPU_ISR_Flash(level);
			if(the_thread->current_priority<_Thread_Heir->current_priority) {
				_Thread_Heir = the_thread;
				if(_Thread_Executing->is_preemptible
					|| the_thread->current_priority==0)
				_Context_Switch_necessary = TRUE;
			}
		}
	}
	_CPU_ISR_Restore(level);
}

void _Thread_Load_environment(Thread_Control *the_thread)
{
	u32 stackbase,sp,size;
	u32 r2,r13,msr_value;
	
	the_thread->Registers.FPSCR = 0x000000f8;

	stackbase = (u32)the_thread->stack;
	size = the_thread->size;

	// tag both bottom & head of stack
	*((u32*)stackbase) = 0xDEADBABE;
	sp = stackbase+size-CPU_MINIMUM_STACK_FRAME_SIZE;
	sp &= ~(CPU_STACK_ALIGNMENT-1);
	*((u32*)sp) = 0;
	
	the_thread->Registers.GPR[1] = sp;
	
	msr_value = (MSR_ME|MSR_IR|MSR_DR|MSR_RI);
	if(!(the_thread->isr_level&CPU_MODES_INTERRUPT_MASK))
		msr_value |= MSR_EE;
	
	the_thread->Registers.MSR = msr_value;
	the_thread->Registers.LR = (u32)_Thread_Handler;

	__asm__ __volatile__ ("mr %0,2; mr %1,13" : "=r" ((r2)), "=r" ((r13)));
	the_thread->Registers.GPR[2] = r2;
	the_thread->Registers.GPR[13] = r13;

#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Load_environment(%p,%p,%d,%p)\n",the_thread,(void*)stackbase,size,(void*)sp);
#endif

}

void _Thread_Ready(Thread_Control *the_thread)
{
	u32 level;
	Thread_Control *heir;

	_CPU_ISR_Disable(level);
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Ready(%p)\n",the_thread);
#endif
	the_thread->current_state = STATES_READY;
	_Priority_Add_to_bit_map(&the_thread->Priority_map);
	_Chain_Append_unprotected(the_thread->ready,&the_thread->Object.Node);
	_CPU_ISR_Flash(level);

	_Thread_Calculate_heir();
	heir = _Thread_Heir;
	if(!(_Thread_Is_executing(heir)) && _Thread_Executing->is_preemptible)
		_Context_Switch_necessary = TRUE;
	
	_CPU_ISR_Restore(level);
}

u32 _Thread_Initialize(Thread_Control *the_thread,void *stack_area,u32 stack_size,u32 priority,u32 isr_level,bool is_preemptible)
{
	u32 actual_stack_size = 0;

#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Initialize(%p,%p,%d,%d,%d)\n",the_thread,stack_area,stack_size,priority,isr_level);
#endif

	if(!stack_area) {
		if(!_Stack_Is_enough(stack_size))
			actual_stack_size = CPU_MINIMUM_STACK_SIZE;
		else
			actual_stack_size = stack_size;

		actual_stack_size = _Thread_Stack_Allocate(the_thread,actual_stack_size);
		if(!actual_stack_size) return 0;

		the_thread->core_allocated_stack = TRUE;
	} else {
		the_thread->stack = stack_area;
		actual_stack_size = stack_size;
		the_thread->core_allocated_stack = FALSE;
	}
	the_thread->size = actual_stack_size;

	_Thread_queue_Initialize(&the_thread->join_list,THREAD_QUEUE_DISCIPLINE_FIFO,STATES_WAITING_FOR_JOIN_AT_EXIT,0);

	memset(&the_thread->Registers,0,sizeof(the_thread->Registers));
	memset(&the_thread->Wait,0,sizeof(the_thread->Wait));

	the_thread->budget_algorithm = (priority<128 ? THREAD_CPU_BUDGET_ALGORITHM_NONE : THREAD_CPU_BUDGET_ALGORITHM_RESET_TIMESLICE);
	the_thread->is_preemptible = is_preemptible;
	the_thread->isr_level = isr_level;
	the_thread->real_priority = priority;
	the_thread->current_state = STATES_DORMANT;
	the_thread->cpu_time_budget = _Thread_Ticks_per_timeslice;
	the_thread->suspend_count = 0;
	the_thread->resource_count = 0;
	_Thread_Set_priority(the_thread,priority);

	libc_create_hook(_Thread_Executing,the_thread);

	return 1;
}

void _Thread_Close(Thread_Control *the_thread)
{
	u32 level;
	void **value_ptr;
	Thread_Control *p;

	_Thread_Set_state(the_thread,STATES_TRANSIENT);
	
	if(!_Thread_queue_Extract_with_proxy(the_thread)) {
		if(_Watchdog_Is_active(&the_thread->Timer))
			_Watchdog_Remove_ticks(&the_thread->Timer);
	}
	
	_CPU_ISR_Disable(level);
	value_ptr = (void**)the_thread->Wait.return_argument;
	while((p=_Thread_queue_Dequeue(&the_thread->join_list))!=NULL) {
		*(void**)p->Wait.return_argument = value_ptr;
	}
	the_thread->cpu_time_budget = 0;
	the_thread->budget_algorithm = THREAD_CPU_BUDGET_ALGORITHM_NONE;
	_CPU_ISR_Restore(level);

	libc_delete_hook(_Thread_Executing,the_thread);

	if(_Thread_Is_allocated_fp(the_thread))
		_Thread_Deallocate_fp();

	_Thread_Stack_Free(the_thread);

	_Objects_Close(the_thread->Object.information,&the_thread->Object);
	_Objects_Free(the_thread->Object.information,&the_thread->Object);
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
		header = &_Thread_Ready_chain[i];
		ptr = (Thread_Control*)header->first;
		while(ptr!=(Thread_Control*)_Chain_Tail(&_Thread_Ready_chain[i])) {
			next = (Thread_Control*)ptr->Object.Node.next;
			if(ptr!=_Thread_Executing)
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
	_Thread_Executing->Wait.return_argument = (u32*)value_ptr;
	_Thread_Close(_Thread_Executing);
	_Thread_Enable_dispatch();
}

u32 _Thread_Start(Thread_Control *the_thread,void* (*entry_point)(void*),void *pointer_argument)
{
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Start(%p,%p,%p,%d)\n",the_thread,entry_point,pointer_argument,the_thread->cur_state);
#endif
	if(_States_Is_dormant(the_thread->current_state)) {
		the_thread->entry_point = entry_point;
		the_thread->pointer_argument = pointer_argument;
		_Thread_Load_environment(the_thread);
		_Thread_Ready(the_thread);
		libc_start_hook(_Thread_Executing,the_thread);
		return 1;
	}
	return 0;
}

void _Thread_Start_multitasking()
{
	_lwp_exitfunc = NULL;

	_System_state_Set(SYSTEM_STATE_BEGIN_MULTITASKING);
	_System_state_Set(SYSTEM_STATE_UP);

	_Context_Switch_necessary = FALSE;
	_Thread_Executing = _Thread_Heir;
#ifdef _LWPTHREADS_DEBUG
	kprintf("_Thread_Start_multitasking(%p,%p)\n",_Thread_Executing,_Thread_Heir);
#endif
	_Watchdog_Insert_ticks(&_lwp_wd_timeslice,millisecs_to_ticks(1));
	_CPU_Context_switch((void*)&_Thread_BSP_context,(void*)&_Thread_Heir->Registers);

	if(_lwp_exitfunc) _lwp_exitfunc();
}

void _Thread_Stop_multitasking(void (*exitfunc)())
{
	_lwp_exitfunc = exitfunc;
	if(_System_state_Get()!=SYSTEM_STATE_SHUTDOWN) {
		_Watchdog_Remove_ticks(&_lwp_wd_timeslice);
		_System_state_Set(SYSTEM_STATE_SHUTDOWN);
		_CPU_Context_switch((void*)&_Thread_Executing->Registers,(void*)&_Thread_BSP_context);
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
	
	_Context_Switch_necessary = FALSE;
	_Thread_Executing = NULL;
	_Thread_Heir = NULL;
	_Thread_Allocated_fp = NULL;
	_Thread_Ticks_per_timeslice = 10;

	memset(&_Thread_BSP_context,0,sizeof(_Thread_BSP_context));

	for(index=0;index<=LWP_PRIO_MAX;index++)
		_Chain_Initialize_empty(&_Thread_Ready_chain[index]);
	
	_System_state_Set(SYSTEM_STATE_BEFORE_MULTITASKING);
}

