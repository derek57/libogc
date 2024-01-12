#ifndef __LWP_MUTEX_INL__
#define __LWP_MUTEX_INL__

static __inline__ u32 _CORE_mutex_Is_locked(CORE_mutex_Control *the_mutex)
{
	return (the_mutex->lock==LWP_MUTEX_LOCKED);
}

static __inline__ u32 _CORE_mutex_Is_priority(CORE_mutex_Attributes *the_attribute)
{
	return (the_attribute->discipline==LWP_MUTEX_PRIORITY);
}

static __inline__ u32 _CORE_mutex_Is_fifo(CORE_mutex_Attributes *the_attribute)
{
	return (the_attribute->discipline==LWP_MUTEX_FIFO);
}

static __inline__ u32 _CORE_mutex_Is_inherit_priority(CORE_mutex_Attributes *the_attribute)
{
	return (the_attribute->discipline==LWP_MUTEX_INHERITPRIO);
}

static __inline__ u32 _CORE_mutex_Is_priority_ceiling(CORE_mutex_Attributes *the_attribute)
{
	return (the_attribute->discipline==LWP_MUTEX_PRIORITYCEIL);
}

static __inline__ u32 _CORE_mutex_Seize_interrupt_trylock(CORE_mutex_Control *the_mutex,u32 *level_p)
{
	Thread_Control *executing;
	u32 level = *level_p;

	executing = _Thread_Executing;
	executing->Wait.return_code = LWP_MUTEX_SUCCESSFUL;
	if(!_CORE_mutex_Is_locked(the_mutex)) {
		the_mutex->lock = LWP_MUTEX_LOCKED;
		the_mutex->holder = executing;
		the_mutex->nest_count = 1;
		if(_CORE_mutex_Is_inherit_priority(&the_mutex->Attributes) || _CORE_mutex_Is_priority_ceiling(&the_mutex->Attributes))
			executing->resource_count++;
		if(!_CORE_mutex_Is_priority_ceiling(&the_mutex->Attributes)) {
			_CPU_ISR_Restore(level);
			return 0;
		}
		{
			u32 prioceiling,priocurr;
			
			prioceiling = the_mutex->Attributes.priority_ceiling;
			priocurr = executing->current_priority;
			if(priocurr==prioceiling) {
				_CPU_ISR_Restore(level);
				return 0;
			}
			if(priocurr>prioceiling) {
				_Thread_Disable_dispatch();
				_CPU_ISR_Restore(level);
				_Thread_Change_priority(the_mutex->holder,the_mutex->Attributes.priority_ceiling,FALSE);
				_Thread_Enable_dispatch();
				return 0;
			}
			executing->Wait.return_code = LWP_MUTEX_CEILINGVIOL;
			the_mutex->nest_count = 0;
			executing->resource_count--;
			_CPU_ISR_Restore(level);
			return 0;
		}
		return 0;
	}

	if(_Thread_Is_executing(the_mutex->holder)) {
		switch(the_mutex->Attributes.lock_nesting_behavior) {
			case LWP_MUTEX_NEST_ACQUIRE:
				the_mutex->nest_count++;
				_CPU_ISR_Restore(level);
				return 0;
			case LWP_MUTEX_NEST_ERROR:
				executing->Wait.return_code = LWP_MUTEX_NEST_NOTALLOWED;
				_CPU_ISR_Restore(level);
				return 0;
			case LWP_MUTEX_NEST_BLOCK:
				break;
		}
	}
	return 1;
}

#endif
