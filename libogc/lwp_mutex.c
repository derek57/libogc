#include "asm.h"
#include "lwp_mutex.h"

void _CORE_mutex_Initialize(CORE_mutex_Control *the_mutex,CORE_mutex_Attributes *the_mutex_attributes,u32 initial_lock)
{
	the_mutex->Attributes = *the_mutex_attributes;
	the_mutex->lock = initial_lock;
	the_mutex->blocked_count = 0;
	
	if(initial_lock==LWP_MUTEX_LOCKED) {
		the_mutex->nest_count = 1;
		the_mutex->holder = _Thread_Executing;
		if(_CORE_mutex_Is_inherit_priority(the_mutex_attributes) || _CORE_mutex_Is_priority_ceiling(the_mutex_attributes))
			_Thread_Executing->resource_count++;
	} else {
		the_mutex->nest_count = 0;
		the_mutex->holder = NULL;
	}

	_Thread_queue_Initialize(&the_mutex->Wait_queue,_CORE_mutex_Is_fifo(the_mutex_attributes)?THREAD_QUEUE_DISCIPLINE_FIFO:THREAD_QUEUE_DISCIPLINE_PRIORITY,STATES_WAITING_FOR_MUTEX,LWP_MUTEX_TIMEOUT);
}

u32 _CORE_mutex_Surrender(CORE_mutex_Control *the_mutex)
{
	Thread_Control *the_thread;
	Thread_Control *holder;

	holder = the_mutex->holder;

	if(the_mutex->Attributes.only_owner_release) {
		if(!_Thread_Is_executing(holder))
			return LWP_MUTEX_NOTOWNER;
	}

	if(!the_mutex->nest_count)
		return LWP_MUTEX_SUCCESSFUL;

	the_mutex->nest_count--;
	if(the_mutex->nest_count!=0) {
		switch(the_mutex->Attributes.lock_nesting_behavior) {
			case LWP_MUTEX_NEST_ACQUIRE:
				return LWP_MUTEX_SUCCESSFUL;
			case LWP_MUTEX_NEST_ERROR:
				return LWP_MUTEX_NEST_NOTALLOWED;
			case LWP_MUTEX_NEST_BLOCK:
				break;
		}
	}

	if(_CORE_mutex_Is_inherit_priority(&the_mutex->Attributes) || _CORE_mutex_Is_priority_ceiling(&the_mutex->Attributes))
		holder->resource_count--;

	the_mutex->holder = NULL;
	if(_CORE_mutex_Is_inherit_priority(&the_mutex->Attributes) || _CORE_mutex_Is_priority_ceiling(&the_mutex->Attributes)) {
		if(holder->resource_count==0 && holder->real_priority!=holder->current_priority) 
			_Thread_Change_priority(holder,holder->real_priority,TRUE);
	}
	
	if((the_thread=_Thread_queue_Dequeue(&the_mutex->Wait_queue))) {
		the_mutex->nest_count = 1;
		the_mutex->holder = the_thread;
		if(_CORE_mutex_Is_inherit_priority(&the_mutex->Attributes) || _CORE_mutex_Is_priority_ceiling(&the_mutex->Attributes))
			the_thread->resource_count++;
	} else
		the_mutex->lock = LWP_MUTEX_UNLOCKED;

	return LWP_MUTEX_SUCCESSFUL;
}

void _CORE_mutex_Seize_interrupt_blocking(CORE_mutex_Control *the_mutex,u64 timeout)
{
	Thread_Control *executing;

	executing = _Thread_Executing;
	if(_CORE_mutex_Is_inherit_priority(&the_mutex->Attributes)){
		if(the_mutex->holder->current_priority>executing->current_priority)
			_Thread_Change_priority(the_mutex->holder,executing->current_priority,FALSE);
	}

	the_mutex->blocked_count++;
	_Thread_queue_Enqueue(&the_mutex->Wait_queue,timeout);

	if(_Thread_Executing->Wait.return_code==LWP_MUTEX_SUCCESSFUL) {
		if(_CORE_mutex_Is_priority_ceiling(&the_mutex->Attributes)) {
			if(the_mutex->Attributes.priority_ceiling<executing->current_priority) 
				_Thread_Change_priority(executing,the_mutex->Attributes.priority_ceiling,FALSE);
		}
	}
	_Thread_Enable_dispatch();
}

void _CORE_mutex_Flush(CORE_mutex_Control *the_mutex,u32 status)
{
	_Thread_queue_Flush(&the_mutex->Wait_queue,status);
}
