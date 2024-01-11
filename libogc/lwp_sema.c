#include "asm.h"
#include "lwp_sema.h"

void CORE_semaphore_Initialize(CORE_semaphore_Control *the_semaphore,CORE_semaphore_Attributes *the_semaphore_attributes,u32 initial_value)
{
	the_semaphore->Attributes = *the_semaphore_attributes;
	the_semaphore->count = initial_value;

	_Thread_queue_Initialize(&the_semaphore->Wait_queue,_CORE_semaphore_Is_priority(the_semaphore_attributes)?LWP_THREADQ_MODEPRIORITY:LWP_THREADQ_MODEFIFO,LWP_STATES_WAITING_FOR_SEMAPHORE,LWP_SEMA_TIMEOUT);
}

u32 _CORE_semaphore_Surrender(CORE_semaphore_Control *the_semaphore,u32 id)
{
	u32 level,status;
	Thread_Control *the_thread;
	
	status = LWP_SEMA_SUCCESSFUL;
	if((the_thread=_Thread_queue_Dequeue(&the_semaphore->Wait_queue))) return status;
	else {
		_CPU_ISR_Disable(level);
		if(the_semaphore->count<=the_semaphore->Attributes.maximum_count)
			++the_semaphore->count;
		else
			status = LWP_SEMA_MAXCNT_EXCEEDED;
		_CPU_ISR_Restore(level);
	}
	return status;
}

u32 _CORE_semaphore_Seize(CORE_semaphore_Control *the_semaphore,u32 id,u32 wait,u64 timeout)
{
	u32 level;
	Thread_Control *executing;
	
	executing = _Thread_Executing;
	executing->Wait.return_code = LWP_SEMA_SUCCESSFUL;

	_CPU_ISR_Disable(level);
	if(the_semaphore->count!=0) {
		--the_semaphore->count;
		_CPU_ISR_Restore(level);
		return LWP_SEMA_SUCCESSFUL;
	}

	if(!wait) {
		_CPU_ISR_Restore(level);
		executing->Wait.return_code = LWP_SEMA_UNSATISFIED_NOWAIT;
		return LWP_SEMA_UNSATISFIED_NOWAIT;
	}

	_Thread_queue_Enter_critical_section(&the_semaphore->Wait_queue);
	executing->Wait.queue = &the_semaphore->Wait_queue;
	executing->Wait.id = id;
	_CPU_ISR_Restore(level);
	
	_Thread_queue_Enqueue(&the_semaphore->Wait_queue,timeout);
	return LWP_SEMA_SUCCESSFUL;
}

void _CORE_semaphore_Flush(CORE_semaphore_Control *the_semaphore,u32 status)
{
	_Thread_queue_Flush(&the_semaphore->Wait_queue,status);
}
