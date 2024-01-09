#ifndef __SYS_STATE_INL__
#define __SYS_STATE_INL__

static __inline__ void _System_state_Handler_initialization()
{
	_sys_state_curr = SYS_STATE_BEFORE_INIT;
}

static __inline__ void _System_state_Set(u32 sys_state)
{
	_sys_state_curr = sys_state;
}

static __inline__ u32 _System_state_Get()
{
	return _sys_state_curr;
}

static __inline__ u32 _System_state_Is_before_initialization(u32 statecode)
{
	return (statecode==SYS_STATE_BEFORE_INIT);
}

static __inline__ u32 _System_state_Is_before_multitasking(u32 statecode)
{
	return (statecode==SYS_STATE_BEFORE_MT);
}

static __inline__ u32 _System_state_Is_begin_multitasking(u32 statecode)
{
	return (statecode==SYS_STATE_BEGIN_MT);
}

static __inline__ u32 _System_state_Is_up(u32 statecode)
{
	return (statecode==SYS_STATE_UP);
}

static __inline__ u32 _System_state_Is_failed(u32 statecode)
{
	return (statecode==SYS_STATE_FAILED);
}

#endif
