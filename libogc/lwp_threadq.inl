#ifndef __LWP_THREADQ_INL__
#define __LWP_THREADQ_INL__

static __inline__ void _Thread_queue_Enter_critical_section(lwp_thrqueue *queue)
{
	queue->sync_state = LWP_THREADQ_NOTHINGHAPPEND;
}

#endif
