#ifndef __LWP_STATES_INL__
#define __LWP_STATES_INL__

#include "lwp_config.h"

RTEMS_INLINE_ROUTINE u32 _States_Set(u32 current_state,u32 states_to_set)
{
	return (current_state|states_to_set);
}

RTEMS_INLINE_ROUTINE u32 _States_Clear(u32 current_state,u32 states_to_clear)
{
	return (current_state&~states_to_clear);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_ready(u32 the_states)
{
	return (the_states==STATES_READY);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_only_dormant(u32 the_states)
{
	return (the_states==STATES_DORMANT);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_dormant(u32 the_states)
{
	return (the_states&STATES_DORMANT);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_suspended(u32 the_states)
{
	return (the_states&STATES_SUSPENDED);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_Transient(u32 the_states)
{
	return (the_states&STATES_TRANSIENT);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_delaying(u32 the_states)
{
	return (the_states&STATES_DELAYING);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_buffer(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_BUFFER);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_segment(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_SEGMENT);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_message(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_MESSAGE);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_event(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_EVENT);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_mutex(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_MUTEX);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_semaphore(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_SEMAPHORE);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_time(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_TIME);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_rpc_reply(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_RPC_REPLY);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_for_period(u32 the_states)
{
	return (the_states&STATES_WAITING_FOR_PERIOD);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_locally_blocked(u32 the_states)
{
	return (the_states&STATES_LOCALLY_BLOCKED);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_waiting_on_thread_queue(u32 the_states)
{
	return (the_states&STATES_WAITING_ON_THREAD_QUEUE);
}

RTEMS_INLINE_ROUTINE u32 _States_Is_blocked(u32 the_states)
{
	return (the_states&STATES_BLOCKED);
}

RTEMS_INLINE_ROUTINE u32 _States_Are_set(u32 the_states,u32 mask)
{
	return ((the_states&mask)!=STATES_READY);
}

#endif
