#ifndef __LWP_STATES_INL__
#define __LWP_STATES_INL__

static __inline__ u32 _States_Set(u32 current_state,u32 states_to_set)
{
	return (current_state|states_to_set);
}

static __inline__ u32 _States_Clear(u32 current_state,u32 states_to_clear)
{
	return (current_state&~states_to_clear);
}

static __inline__ u32 _States_Is_ready(u32 the_states)
{
	return (the_states==LWP_STATES_READY);
}

static __inline__ u32 _States_Is_only_dormant(u32 the_states)
{
	return (the_states==LWP_STATES_DORMANT);
}

static __inline__ u32 _States_Is_dormant(u32 the_states)
{
	return (the_states&LWP_STATES_DORMANT);
}

static __inline__ u32 _States_Is_suspended(u32 the_states)
{
	return (the_states&LWP_STATES_SUSPENDED);
}

static __inline__ u32 _States_Is_Transient(u32 the_states)
{
	return (the_states&LWP_STATES_TRANSIENT);
}

static __inline__ u32 _States_Is_delaying(u32 the_states)
{
	return (the_states&LWP_STATES_DELAYING);
}

static __inline__ u32 _States_Is_waiting_for_buffer(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_BUFFER);
}

static __inline__ u32 _States_Is_waiting_for_segment(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_SEGMENT);
}

static __inline__ u32 _States_Is_waiting_for_message(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_MESSAGE);
}

static __inline__ u32 _States_Is_waiting_for_event(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_EVENT);
}

static __inline__ u32 _States_Is_waiting_for_mutex(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_MUTEX);
}

static __inline__ u32 _States_Is_waiting_for_semaphore(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_SEMAPHORE);
}

static __inline__ u32 _States_Is_waiting_for_time(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_TIME);
}

static __inline__ u32 _States_Is_waiting_for_rpc_reply(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_RPCREPLAY);
}

static __inline__ u32 _States_Is_waiting_for_period(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_FOR_PERIOD);
}

static __inline__ u32 _States_Is_locally_blocked(u32 the_states)
{
	return (the_states&LWP_STATES_LOCALLY_BLOCKED);
}

static __inline__ u32 _States_Is_waiting_on_thread_queue(u32 the_states)
{
	return (the_states&LWP_STATES_WAITING_ON_THREADQ);
}

static __inline__ u32 _States_Is_blocked(u32 the_states)
{
	return (the_states&LWP_STATES_BLOCKED);
}

static __inline__ u32 _States_Are_set(u32 the_states,u32 mask)
{
	return ((the_states&mask)!=LWP_STATES_READY);
}

#endif
