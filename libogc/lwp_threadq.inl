#ifndef __LWP_THREADQ_INL__
#define __LWP_THREADQ_INL__

RTEMS_INLINE_ROUTINE void _Thread_queue_Enter_critical_section(Thread_queue_Control *the_thread_queue)
{
	the_thread_queue->sync_state = THREAD_QUEUE_NOTHING_HAPPENED;
}

#endif
