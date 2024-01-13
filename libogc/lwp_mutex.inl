#ifndef __LWP_MUTEX_INL__
#define __LWP_MUTEX_INL__

static __inline__ u32 _CORE_mutex_Is_locked(CORE_mutex_Control *the_mutex)
{
	return (the_mutex->lock==CORE_MUTEX_LOCKED);
}

static __inline__ u32 _CORE_mutex_Is_priority(CORE_mutex_Attributes *the_attribute)
{
	return (the_attribute->discipline==CORE_MUTEX_DISCIPLINES_PRIORITY);
}

static __inline__ u32 _CORE_mutex_Is_fifo(CORE_mutex_Attributes *the_attribute)
{
	return (the_attribute->discipline==CORE_MUTEX_DISCIPLINES_FIFO);
}

static __inline__ u32 _CORE_mutex_Is_inherit_priority(CORE_mutex_Attributes *the_attribute)
{
	return (the_attribute->discipline==CORE_MUTEX_DISCIPLINES_PRIORITY_INHERIT);
}

static __inline__ u32 _CORE_mutex_Is_priority_ceiling(CORE_mutex_Attributes *the_attribute)
{
	return (the_attribute->discipline==CORE_MUTEX_DISCIPLINES_PRIORITY_CEILING);
}

static __inline__ u32 _CORE_mutex_Seize_interrupt_trylock(CORE_mutex_Control *the_mutex,u32 *level_p)
{
	Thread_Control *executing;
	u32 level = *level_p;

	executing = _Thread_Executing;
	executing->Wait.return_code = CORE_MUTEX_STATUS_SUCCESSFUL;
	if(!_CORE_mutex_Is_locked(the_mutex)) {
		the_mutex->lock = CORE_MUTEX_LOCKED;
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
			executing->Wait.return_code = CORE_MUTEX_STATUS_CEILING_VIOLATED;
			the_mutex->nest_count = 0;
			executing->resource_count--;
			_CPU_ISR_Restore(level);
			return 0;
		}
		return 0;
	}

	if(_Thread_Is_executing(the_mutex->holder)) {
		switch(the_mutex->Attributes.lock_nesting_behavior) {
			case CORE_MUTEX_NESTING_ACQUIRES:
				the_mutex->nest_count++;
				_CPU_ISR_Restore(level);
				return 0;
			case CORE_MUTEX_NESTING_IS_ERROR:
				executing->Wait.return_code = CORE_MUTEX_STATUS_NESTING_NOT_ALLOWED;
				_CPU_ISR_Restore(level);
				return 0;
			case CORE_MUTEX_NESTING_BLOCKS:
				break;
		}
	}
	return 1;
}

#endif
