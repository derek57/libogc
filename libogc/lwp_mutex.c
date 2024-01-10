#include "asm.h"
#include "lwp_mutex.h"

void _CORE_mutex_Initialize(CORE_mutex_Control *mutex,CORE_mutex_Attributes *attrs,u32 init_lock)
{
	mutex->Attributes = *attrs;
	mutex->lock = init_lock;
	mutex->blocked_count = 0;
	
	if(init_lock==LWP_MUTEX_LOCKED) {
		mutex->nest_count = 1;
		mutex->holder = _thr_executing;
		if(_CORE_mutex_Is_inherit_priority(attrs) || _CORE_mutex_Is_priority_ceiling(attrs))
			_thr_executing->resource_count++;
	} else {
		mutex->nest_count = 0;
		mutex->holder = NULL;
	}

	_Thread_queue_Initialize(&mutex->Wait_queue,_CORE_mutex_Is_fifo(attrs)?LWP_THREADQ_MODEFIFO:LWP_THREADQ_MODEPRIORITY,LWP_STATES_WAITING_FOR_MUTEX,LWP_MUTEX_TIMEOUT);
}

u32 _CORE_mutex_Surrender(CORE_mutex_Control *mutex)
{
	Thread_Control *thethread;
	Thread_Control *holder;

	holder = mutex->holder;

	if(mutex->Attributes.only_owner_release) {
		if(!_Thread_Is_executing(holder))
			return LWP_MUTEX_NOTOWNER;
	}

	if(!mutex->nest_count)
		return LWP_MUTEX_SUCCESSFUL;

	mutex->nest_count--;
	if(mutex->nest_count!=0) {
		switch(mutex->Attributes.lock_nesting_behavior) {
			case LWP_MUTEX_NEST_ACQUIRE:
				return LWP_MUTEX_SUCCESSFUL;
			case LWP_MUTEX_NEST_ERROR:
				return LWP_MUTEX_NEST_NOTALLOWED;
			case LWP_MUTEX_NEST_BLOCK:
				break;
		}
	}

	if(_CORE_mutex_Is_inherit_priority(&mutex->Attributes) || _CORE_mutex_Is_priority_ceiling(&mutex->Attributes))
		holder->resource_count--;

	mutex->holder = NULL;
	if(_CORE_mutex_Is_inherit_priority(&mutex->Attributes) || _CORE_mutex_Is_priority_ceiling(&mutex->Attributes)) {
		if(holder->resource_count==0 && holder->real_priority!=holder->current_priority) 
			_Thread_Change_priority(holder,holder->real_priority,TRUE);
	}
	
	if((thethread=_Thread_queue_Dequeue(&mutex->Wait_queue))) {
		mutex->nest_count = 1;
		mutex->holder = thethread;
		if(_CORE_mutex_Is_inherit_priority(&mutex->Attributes) || _CORE_mutex_Is_priority_ceiling(&mutex->Attributes))
			thethread->resource_count++;
	} else
		mutex->lock = LWP_MUTEX_UNLOCKED;

	return LWP_MUTEX_SUCCESSFUL;
}

void _CORE_mutex_Seize_interrupt_blocking(CORE_mutex_Control *mutex,u64 timeout)
{
	Thread_Control *exec;

	exec = _thr_executing;
	if(_CORE_mutex_Is_inherit_priority(&mutex->Attributes)){
		if(mutex->holder->current_priority>exec->current_priority)
			_Thread_Change_priority(mutex->holder,exec->current_priority,FALSE);
	}

	mutex->blocked_count++;
	_Thread_queue_Enqueue(&mutex->Wait_queue,timeout);

	if(_thr_executing->Wait.return_code==LWP_MUTEX_SUCCESSFUL) {
		if(_CORE_mutex_Is_priority_ceiling(&mutex->Attributes)) {
			if(mutex->Attributes.priority_ceiling<exec->current_priority) 
				_Thread_Change_priority(exec,mutex->Attributes.priority_ceiling,FALSE);
		}
	}
	_Thread_Enable_dispatch();
}

void _CORE_mutex_Flush(CORE_mutex_Control *mutex,u32 status)
{
	_Thread_queue_Flush(&mutex->Wait_queue,status);
}
