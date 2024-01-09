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

lwp_cntrl* _Thread_queue_First_fifo(lwp_thrqueue *queue);
lwp_cntrl* _Thread_queue_First_priority(lwp_thrqueue *queue);
void _Thread_queue_Enqueue_fifo(lwp_thrqueue *queue,lwp_cntrl *thethread,u64 timeout);
lwp_cntrl* _Thread_queue_Dequeue_fifo(lwp_thrqueue *queue);
void _Thread_queue_Enqueue_priority(lwp_thrqueue *queue,lwp_cntrl *thethread,u64 timeout);
lwp_cntrl* _Thread_queue_Dequeue_priority(lwp_thrqueue *queue);
void _Thread_queue_Initialize(lwp_thrqueue *queue,u32 mode,u32 state,u32 timeout_state);
lwp_cntrl* _Thread_queue_First(lwp_thrqueue *queue);
void _Thread_queue_Enqueue(lwp_thrqueue *queue,u64 timeout);
lwp_cntrl* _Thread_queue_Dequeue(lwp_thrqueue *queue);
void _Thread_queue_Flush(lwp_thrqueue *queue,u32 status);
void _Thread_queue_Extract(lwp_thrqueue *queue,lwp_cntrl *thethread);
void _Thread_queue_Extract_fifo(lwp_thrqueue *queue,lwp_cntrl *thethread);
void _Thread_queue_Extract_priority(lwp_thrqueue *queue,lwp_cntrl *thethread);
u32 _Thread_queue_Extract_with_proxy(lwp_cntrl *thethread);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_threadq.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
