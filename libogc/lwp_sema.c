#include "asm.h"
#include "lwp_sema.h"

void CORE_semaphore_Initialize(CORE_semaphore_Control *the_semaphore,CORE_semaphore_Attributes *the_semaphore_attributes,u32 initial_value)
{
	the_semaphore->Attributes = *the_semaphore_attributes;
	the_semaphore->count = initial_value;

	_Thread_queue_Initialize(&the_semaphore->Wait_queue,_CORE_semaphore_Is_priority(the_semaphore_attributes)?THREAD_QUEUE_DISCIPLINE_PRIORITY:THREAD_QUEUE_DISCIPLINE_FIFO,STATES_WAITING_FOR_SEMAPHORE,CORE_SEMAPHORE_TIMEOUT);
}

u32 _CORE_semaphore_Surrender(CORE_semaphore_Control *the_semaphore,u32 id)
{
	u32 level,status;
	Thread_Control *the_thread;
	
	status = CORE_SEMAPHORE_STATUS_SUCCESSFUL;
	if((the_thread=_Thread_queue_Dequeue(&the_semaphore->Wait_queue))) return status;
	else {
		_ISR_Disable(level);
		if(the_semaphore->count<=the_semaphore->Attributes.maximum_count)
			++the_semaphore->count;
		else
			status = CORE_SEMAPHORE_MAXIMUM_COUNT_EXCEEDED;
		_ISR_Enable(level);
	}
	return status;
}

u32 _CORE_semaphore_Seize(CORE_semaphore_Control *the_semaphore,u32 id,u32 wait,u64 timeout)
{
	u32 level;
	Thread_Control *executing;
	
	executing = _Thread_Executing;
	executing->Wait.return_code = CORE_SEMAPHORE_STATUS_SUCCESSFUL;

	_ISR_Disable(level);
	if(the_semaphore->count!=0) {
		--the_semaphore->count;
		_ISR_Enable(level);
		return CORE_SEMAPHORE_STATUS_SUCCESSFUL;
	}

	if(!wait) {
		_ISR_Enable(level);
		executing->Wait.return_code = CORE_SEMAPHORE_STATUS_UNSATISFIED_NOWAIT;
		return CORE_SEMAPHORE_STATUS_UNSATISFIED_NOWAIT;
	}

	_Thread_queue_Enter_critical_section(&the_semaphore->Wait_queue);
	executing->Wait.queue = &the_semaphore->Wait_queue;
	executing->Wait.id = id;
	_ISR_Enable(level);
	
	_Thread_queue_Enqueue(&the_semaphore->Wait_queue,timeout);
	return CORE_SEMAPHORE_STATUS_SUCCESSFUL;
}

void _CORE_semaphore_Flush(CORE_semaphore_Control *the_semaphore,u32 status)
{
	_Thread_queue_Flush(&the_semaphore->Wait_queue,status);
}
