#ifndef __LWP_STATES_INL__
#define __LWP_STATES_INL__

static __inline__ u32 _States_Set(u32 curr_state,u32 stateset)
{
	return (curr_state|stateset);
}

static __inline__ u32 _States_Clear(u32 curr_state,u32 stateclear)
{
	return (curr_state&~stateclear);
}

static __inline__ u32 _States_Is_ready(u32 curr_state)
{
	return (curr_state==LWP_STATES_READY);
}

static __inline__ u32 _States_Is_only_dormant(u32 curr_state)
{
	return (curr_state==LWP_STATES_DORMANT);
}

static __inline__ u32 _States_Is_dormant(u32 curr_state)
{
	return (curr_state&LWP_STATES_DORMANT);
}

static __inline__ u32 _States_Is_suspended(u32 curr_state)
{
	return (curr_state&LWP_STATES_SUSPENDED);
}

static __inline__ u32 _States_Is_Transient(u32 curr_state)
{
	return (curr_state&LWP_STATES_TRANSIENT);
}

static __inline__ u32 _States_Is_delaying(u32 curr_state)
{
	return (curr_state&LWP_STATES_DELAYING);
}

static __inline__ u32 _States_Is_waiting_for_buffer(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_BUFFER);
}

static __inline__ u32 _States_Is_waiting_for_segment(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_SEGMENT);
}

static __inline__ u32 _States_Is_waiting_for_message(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_MESSAGE);
}

static __inline__ u32 _States_Is_waiting_for_event(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_EVENT);
}

static __inline__ u32 _States_Is_waiting_for_mutex(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_MUTEX);
}

static __inline__ u32 _States_Is_waiting_for_semaphore(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_SEMAPHORE);
}

static __inline__ u32 _States_Is_waiting_for_time(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_TIME);
}

static __inline__ u32 _States_Is_waiting_for_rpc_reply(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_RPCREPLAY);
}

static __inline__ u32 _States_Is_waiting_for_period(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_FOR_PERIOD);
}

static __inline__ u32 _States_Is_locally_blocked(u32 curr_state)
{
	return (curr_state&LWP_STATES_LOCALLY_BLOCKED);
}

static __inline__ u32 _States_Is_waiting_on_thread_queue(u32 curr_state)
{
	return (curr_state&LWP_STATES_WAITING_ON_THREADQ);
}

static __inline__ u32 _States_Is_blocked(u32 curr_state)
{
	return (curr_state&LWP_STATES_BLOCKED);
}

static __inline__ u32 _States_Are_set(u32 curr_state,u32 mask)
{
	return ((curr_state&mask)!=LWP_STATES_READY);
}

#endif
