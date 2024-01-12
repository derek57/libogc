#ifndef __SYS_STATE_INL__
#define __SYS_STATE_INL__

static __inline__ void _System_state_Handler_initialization()
{
	_System_state_Current = SYS_STATE_BEFORE_INIT;
}

static __inline__ void _System_state_Set(u32 state)
{
	_System_state_Current = state;
}

static __inline__ u32 _System_state_Get()
{
	return _System_state_Current;
}

static __inline__ u32 _System_state_Is_before_initialization(u32 state)
{
	return (state==SYS_STATE_BEFORE_INIT);
}

static __inline__ u32 _System_state_Is_before_multitasking(u32 state)
{
	return (state==SYS_STATE_BEFORE_MT);
}

static __inline__ u32 _System_state_Is_begin_multitasking(u32 state)
{
	return (state==SYS_STATE_BEGIN_MT);
}

static __inline__ u32 _System_state_Is_up(u32 state)
{
	return (state==SYS_STATE_UP);
}

static __inline__ u32 _System_state_Is_failed(u32 state)
{
	return (state==SYS_STATE_FAILED);
}

#endif
