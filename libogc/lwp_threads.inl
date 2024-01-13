#ifndef __LWP_INL__
#define __LWP_INL__

static __inline__ u32 _Thread_Is_executing(Thread_Control *the_thread)
{
	return (the_thread==_Thread_Executing);
}

static __inline__ u32 _Thread_Is_heir(Thread_Control *the_thread)
{
	return (the_thread==_Thread_Heir);
}

static __inline__ void _Thread_Calculate_heir()
{
	_Thread_Heir = (Thread_Control*)_Thread_Ready_chain[_Priority_Get_highest()].first;
#ifdef _LWPTHREADS_DEBUG
	printf("_Thread_Calculate_heir(%p)\n",_Thread_Heir);
#endif
}

static __inline__ u32 _Thread_Is_allocated_fp(Thread_Control *the_thread)
{
	return (the_thread==_Thread_Allocated_fp);
}

static __inline__ void _Thread_Deallocate_fp()
{
	_Thread_Allocated_fp = NULL;
}

static __inline__ void _Thread_Dispatch_initialization()
{
	_Thread_Dispatch_disable_level = 1;
}

static __inline__ void _Thread_Enable_dispatch()
{
	if((--_Thread_Dispatch_disable_level)==0)
		_Thread_Dispatch();
}

static __inline__ void _Thread_Disable_dispatch()
{
	++_Thread_Dispatch_disable_level;
}

static __inline__ void _Thread_Unnest_dispatch()
{
	--_Thread_Dispatch_disable_level;
}

static __inline__ void _Thread_Unblock(Thread_Control *the_thread)
{
	_Thread_Clear_state(the_thread,STATES_BLOCKED);
}

static __inline__ void** _Thread_Get_libc_reent()
{
	return _Thread_libc_reent;
}

static __inline__ void _Thread_Set_libc_reent(void **libc_reent)
{
	_Thread_libc_reent = libc_reent;
}

static __inline__ bool _Thread_Is_context_switch_necessary()
{

	return _Context_Switch_necessary;
}

static __inline__ bool _Thread_Is_dispatching_enabled()
{
	return (_Thread_Dispatch_disable_level==0);
}
#endif
