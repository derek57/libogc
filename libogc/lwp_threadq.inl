#ifndef __LWP_THREADQ_INL__
#define __LWP_THREADQ_INL__

static __inline__ void _Thread_queue_Enter_critical_section(Thread_queue_Control *the_thread_queue)
{
	the_thread_queue->sync_state = LWP_THREADQ_NOTHINGHAPPEND;
}

#endif
