#include "asm.h"
#include "lwp_sema.h"

void CORE_semaphore_Initialize(CORE_semaphore_Control *sema,CORE_semaphore_Attributes *attrs,u32 init_count)
{
	sema->attrs = *attrs;
	sema->count = init_count;

	_Thread_queue_Initialize(&sema->wait_queue,_CORE_semaphore_Is_priority(attrs)?LWP_THREADQ_MODEPRIORITY:LWP_THREADQ_MODEFIFO,LWP_STATES_WAITING_FOR_SEMAPHORE,LWP_SEMA_TIMEOUT);
}

u32 _CORE_semaphore_Surrender(CORE_semaphore_Control *sema,u32 id)
{
	u32 level,ret;
	Thread_Control *thethread;
	
	ret = LWP_SEMA_SUCCESSFUL;
	if((thethread=_Thread_queue_Dequeue(&sema->wait_queue))) return ret;
	else {
		_CPU_ISR_Disable(level);
		if(sema->count<=sema->attrs.max_cnt)
			++sema->count;
		else
			ret = LWP_SEMA_MAXCNT_EXCEEDED;
		_CPU_ISR_Restore(level);
	}
	return ret;
}

u32 _CORE_semaphore_Seize(CORE_semaphore_Control *sema,u32 id,u32 wait,u64 timeout)
{
	u32 level;
	Thread_Control *exec;
	
	exec = _thr_executing;
	exec->wait.ret_code = LWP_SEMA_SUCCESSFUL;

	_CPU_ISR_Disable(level);
	if(sema->count!=0) {
		--sema->count;
		_CPU_ISR_Restore(level);
		return LWP_SEMA_SUCCESSFUL;
	}

	if(!wait) {
		_CPU_ISR_Restore(level);
		exec->wait.ret_code = LWP_SEMA_UNSATISFIED_NOWAIT;
		return LWP_SEMA_UNSATISFIED_NOWAIT;
	}

	_Thread_queue_Enter_critical_section(&sema->wait_queue);
	exec->wait.queue = &sema->wait_queue;
	exec->wait.id = id;
	_CPU_ISR_Restore(level);
	
	_Thread_queue_Enqueue(&sema->wait_queue,timeout);
	return LWP_SEMA_SUCCESSFUL;
}

void _CORE_semaphore_Flush(CORE_semaphore_Control *sema,u32 status)
{
	_Thread_queue_Flush(&sema->wait_queue,status);
}
