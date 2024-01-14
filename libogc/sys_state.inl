#ifndef __SYS_STATE_INL__
#define __SYS_STATE_INL__

#include "lwp_config.h"

RTEMS_INLINE_ROUTINE void _System_state_Handler_initialization()
{
	_System_state_Current = SYSTEM_STATE_BEFORE_INITIALIZATION;
}

RTEMS_INLINE_ROUTINE void _System_state_Set(u32 state)
{
	_System_state_Current = state;
}

RTEMS_INLINE_ROUTINE u32 _System_state_Get()
{
	return _System_state_Current;
}

RTEMS_INLINE_ROUTINE u32 _System_state_Is_before_initialization(u32 state)
{
	return (state==SYSTEM_STATE_BEFORE_INITIALIZATION);
}

RTEMS_INLINE_ROUTINE u32 _System_state_Is_before_multitasking(u32 state)
{
	return (state==SYSTEM_STATE_BEFORE_MULTITASKING);
}

RTEMS_INLINE_ROUTINE u32 _System_state_Is_begin_multitasking(u32 state)
{
	return (state==SYSTEM_STATE_BEGIN_MULTITASKING);
}

RTEMS_INLINE_ROUTINE u32 _System_state_Is_up(u32 state)
{
	return (state==SYSTEM_STATE_UP);
}

RTEMS_INLINE_ROUTINE u32 _System_state_Is_failed(u32 state)
{
	return (state==SYSTEM_STATE_FAILED);
}

#endif
