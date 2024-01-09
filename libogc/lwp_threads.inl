#ifndef __LWP_INL__
#define __LWP_INL__

static __inline__ u32 _Thread_Is_executing(lwp_cntrl *thethread)
{
	return (thethread==_thr_executing);
}

static __inline__ u32 _Thread_Is_heir(lwp_cntrl *thethread)
{
	return (thethread==_thr_heir);
}

static __inline__ void _Thread_Calculate_heir()
{
	_thr_heir = (lwp_cntrl*)_lwp_thr_ready[_Priority_Get_highest()].first;
#ifdef _LWPTHREADS_DEBUG
	printf("_Thread_Calculate_heir(%p)\n",_thr_heir);
#endif
}

static __inline__ u32 _Thread_Is_allocated_fp(lwp_cntrl *thethread)
{
	return (thethread==_thr_allocated_fp);
}

static __inline__ void _Thread_Deallocate_fp()
{
	_thr_allocated_fp = NULL;
}

static __inline__ void _Thread_Dispatch_initialization()
{
	_thread_dispatch_disable_level = 1;
}

static __inline__ void _Thread_Enable_dispatch()
{
	if((--_thread_dispatch_disable_level)==0)
		_Thread_Dispatch();
}

static __inline__ void _Thread_Disable_dispatch()
{
	++_thread_dispatch_disable_level;
}

static __inline__ void _Thread_Unnest_dispatch()
{
	--_thread_dispatch_disable_level;
}

static __inline__ void _Thread_Unblock(lwp_cntrl *thethread)
{
	_Thread_Clear_state(thethread,LWP_STATES_BLOCKED);
}

static __inline__ void** _Thread_Get_libc_reent()
{
	return __lwp_thr_libc_reent;
}

static __inline__ void _Thread_Set_libc_reent(void **libc_reent)
{
	__lwp_thr_libc_reent = libc_reent;
}

static __inline__ bool _Thread_Is_context_switch_necessary()
{

	return _context_switch_want;
}

static __inline__ bool _Thread_Is_dispatching_enabled()
{
	return (_thread_dispatch_disable_level==0);
}
#endif
