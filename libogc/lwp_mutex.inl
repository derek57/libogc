#ifndef __LWP_MUTEX_INL__
#define __LWP_MUTEX_INL__

static __inline__ u32 _CORE_mutex_Is_locked(CORE_mutex_Control *mutex)
{
	return (mutex->lock==LWP_MUTEX_LOCKED);
}

static __inline__ u32 _CORE_mutex_Is_priority(CORE_mutex_Attributes *attrs)
{
	return (attrs->discipline==LWP_MUTEX_PRIORITY);
}

static __inline__ u32 _CORE_mutex_Is_fifo(CORE_mutex_Attributes *attrs)
{
	return (attrs->discipline==LWP_MUTEX_FIFO);
}

static __inline__ u32 _CORE_mutex_Is_inherit_priority(CORE_mutex_Attributes *attrs)
{
	return (attrs->discipline==LWP_MUTEX_INHERITPRIO);
}

static __inline__ u32 _CORE_mutex_Is_priority_ceiling(CORE_mutex_Attributes *attrs)
{
	return (attrs->discipline==LWP_MUTEX_PRIORITYCEIL);
}

static __inline__ u32 _CORE_mutex_Seize_interrupt_trylock(CORE_mutex_Control *mutex,u32 *isr_level)
{
	Thread_Control *exec;
	u32 level = *isr_level;

	exec = _thr_executing;
	exec->Wait.return_code = LWP_MUTEX_SUCCESSFUL;
	if(!_CORE_mutex_Is_locked(mutex)) {
		mutex->lock = LWP_MUTEX_LOCKED;
		mutex->holder = exec;
		mutex->nest_count = 1;
		if(_CORE_mutex_Is_inherit_priority(&mutex->Attributes) || _CORE_mutex_Is_priority_ceiling(&mutex->Attributes))
			exec->resource_count++;
		if(!_CORE_mutex_Is_priority_ceiling(&mutex->Attributes)) {
			_CPU_ISR_Restore(level);
			return 0;
		}
		{
			u32 prioceiling,priocurr;
			
			prioceiling = mutex->Attributes.priority_ceiling;
			priocurr = exec->current_priority;
			if(priocurr==prioceiling) {
				_CPU_ISR_Restore(level);
				return 0;
			}
			if(priocurr>prioceiling) {
				_Thread_Disable_dispatch();
				_CPU_ISR_Restore(level);
				_Thread_Change_priority(mutex->holder,mutex->Attributes.priority_ceiling,FALSE);
				_Thread_Enable_dispatch();
				return 0;
			}
			exec->Wait.return_code = LWP_MUTEX_CEILINGVIOL;
			mutex->nest_count = 0;
			exec->resource_count--;
			_CPU_ISR_Restore(level);
			return 0;
		}
		return 0;
	}

	if(_Thread_Is_executing(mutex->holder)) {
		switch(mutex->Attributes.lock_nesting_behavior) {
			case LWP_MUTEX_NEST_ACQUIRE:
				mutex->nest_count++;
				_CPU_ISR_Restore(level);
				return 0;
			case LWP_MUTEX_NEST_ERROR:
				exec->Wait.return_code = LWP_MUTEX_NEST_NOTALLOWED;
				_CPU_ISR_Restore(level);
				return 0;
			case LWP_MUTEX_NEST_BLOCK:
				break;
		}
	}
	return 1;
}

#endif
