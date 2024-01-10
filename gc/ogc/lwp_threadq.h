#ifndef __LWP_THREADQ_H__
#define __LWP_THREADQ_H__

#include <gctypes.h>
#include <lwp_tqdata.h>
#include <lwp_threads.h>
#include <lwp_watchdog.h>

#define LWP_THREADQ_NOTIMEOUT		LWP_WD_NOTIMEOUT

#ifdef __cplusplus
extern "C" {
#endif

Thread_Control* _Thread_queue_First_fifo(Thread_queue_Control *queue);
Thread_Control* _Thread_queue_First_priority(Thread_queue_Control *queue);
void _Thread_queue_Enqueue_fifo(Thread_queue_Control *queue,Thread_Control *thethread,u64 timeout);
Thread_Control* _Thread_queue_Dequeue_fifo(Thread_queue_Control *queue);
void _Thread_queue_Enqueue_priority(Thread_queue_Control *queue,Thread_Control *thethread,u64 timeout);
Thread_Control* _Thread_queue_Dequeue_priority(Thread_queue_Control *queue);
void _Thread_queue_Initialize(Thread_queue_Control *queue,u32 mode,u32 state,u32 timeout_state);
Thread_Control* _Thread_queue_First(Thread_queue_Control *queue);
void _Thread_queue_Enqueue(Thread_queue_Control *queue,u64 timeout);
Thread_Control* _Thread_queue_Dequeue(Thread_queue_Control *queue);
void _Thread_queue_Flush(Thread_queue_Control *queue,u32 status);
void _Thread_queue_Extract(Thread_queue_Control *queue,Thread_Control *thethread);
void _Thread_queue_Extract_fifo(Thread_queue_Control *queue,Thread_Control *thethread);
void _Thread_queue_Extract_priority(Thread_queue_Control *queue,Thread_Control *thethread);
u32 _Thread_queue_Extract_with_proxy(Thread_Control *thethread);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_threadq.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
