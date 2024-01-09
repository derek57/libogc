#ifndef __LWP_MUTEX_INL__
#define __LWP_MUTEX_INL__

static __inline__ u32 _CORE_mutex_Is_locked(lwp_mutex *mutex)
{
	return (mutex->lock==LWP_MUTEX_LOCKED);
}

static __inline__ u32 _CORE_mutex_Is_priority(lwp_mutex_attr *attrs)
{
	return (attrs->mode==LWP_MUTEX_PRIORITY);
}

static __inline__ u32 _CORE_mutex_Is_fifo(lwp_mutex_attr *attrs)
{
	return (attrs->mode==LWP_MUTEX_FIFO);
}

static __inline__ u32 _CORE_mutex_Is_inherit_priority(lwp_mutex_attr *attrs)
{
	return (attrs->mode==LWP_MUTEX_INHERITPRIO);
}

static __inline__ u32 _CORE_mutex_Is_priority_ceiling(lwp_mutex_attr *attrs)
{
	return (attrs->mode==LWP_MUTEX_PRIORITYCEIL);
}

static __inline__ u32 _CORE_mutex_Seize_interrupt_trylock(lwp_mutex *mutex,u32 *isr_level)
{
	lwp_cntrl *exec;
	u32 level = *isr_level;

	exec = _thr_executing;
	exec->wait.ret_code = LWP_MUTEX_SUCCESSFUL;
	if(!_CORE_mutex_Is_locked(mutex)) {
		mutex->lock = LWP_MUTEX_LOCKED;
		mutex->holder = exec;
		mutex->nest_cnt = 1;
		if(_CORE_mutex_Is_inherit_priority(&mutex->atrrs) || _CORE_mutex_Is_priority_ceiling(&mutex->atrrs))
			exec->res_cnt++;
		if(!_CORE_mutex_Is_priority_ceiling(&mutex->atrrs)) {
			_CPU_ISR_Restore(level);
			return 0;
		}
		{
			u32 prioceiling,priocurr;
			
			prioceiling = mutex->atrrs.prioceil;
			priocurr = exec->cur_prio;
			if(priocurr==prioceiling) {
				_CPU_ISR_Restore(level);
				return 0;
			}
			if(priocurr>prioceiling) {
				_Thread_Disable_dispatch();
				_CPU_ISR_Restore(level);
				_Thread_Change_priority(mutex->holder,mutex->atrrs.prioceil,FALSE);
				_Thread_Enable_dispatch();
				return 0;
			}
			exec->wait.ret_code = LWP_MUTEX_CEILINGVIOL;
			mutex->nest_cnt = 0;
			exec->res_cnt--;
			_CPU_ISR_Restore(level);
			return 0;
		}
		return 0;
	}

	if(_Thread_Is_executing(mutex->holder)) {
		switch(mutex->atrrs.nest_behavior) {
			case LWP_MUTEX_NEST_ACQUIRE:
				mutex->nest_cnt++;
				_CPU_ISR_Restore(level);
				return 0;
			case LWP_MUTEX_NEST_ERROR:
				exec->wait.ret_code = LWP_MUTEX_NEST_NOTALLOWED;
				_CPU_ISR_Restore(level);
				return 0;
			case LWP_MUTEX_NEST_BLOCK:
				break;
		}
	}
	return 1;
}

#endif
