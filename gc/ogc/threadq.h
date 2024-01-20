/*  threadq.h
 *
 *  This include file contains all the constants and structures associated
 *  with the manipulation of objects.
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 */

#ifndef __THREAD_QUEUE_h
#define __THREAD_QUEUE_h

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <lwp_tqdata.h>
#include <lwp_threads.h>
#include <lwp_watchdog.h>

/*
 *  Constant for indefinite wait.
 */

#define RTEMS_NO_TIMEOUT		WATCHDOG_NO_TIMEOUT

/*PAGE
 *
 *  _Thread_queue_Header_number
 *
 */

#define _Thread_queue_Header_number( _the_priority ) \
    ((_the_priority) / TASK_QUEUE_DATA_PRIORITIES_PER_HEADER)

/*PAGE
 *
 *  _Thread_queue_Is_reverse_search
 *
 */

#define _Thread_queue_Is_reverse_search( _the_priority ) \
     ( (_the_priority) & TASK_QUEUE_DATA_REVERSE_SEARCH_MASK )

/*
 *  _Thread_queue_Dequeue
 *
 *  DESCRIPTION:
 *
 *  This function returns a pointer to a thread waiting on
 *  the_thread_queue.  The selection of this thread is based on
 *  the discipline of the_thread_queue.  If no threads are waiting
 *  on the_thread_queue, then NULL is returned.
 */

Thread_Control *_Thread_queue_Dequeue(
  Thread_queue_Control *the_thread_queue
);

/*
 *  _Thread_queue_Enqueue
 *
 *  DESCRIPTION:
 *
 *  This routine enqueues the currently executing thread on
 *  the_thread_queue with an optional timeout.
 */

void _Thread_queue_Enqueue(
  Thread_queue_Control *the_thread_queue,
  Watchdog_Interval     timeout
);

/*
 *  _Thread_queue_Extract
 *
 *  DESCRIPTION:
 *
 *  This routine removes the_thread from the_thread_queue
 *  and cancels any timeouts associated with this blocking.
 */

void _Thread_queue_Extract(
  Thread_queue_Control *the_thread_queue,
  Thread_Control       *the_thread
);

/*
 *  _Thread_queue_Extract_with_proxy
 *
 *  DESCRIPTION:
 *
 *  This routine extracts the_thread from the_thread_queue
 *  and insures that if there is a proxy for this task on 
 *  another node, it is also dealt with.
 */
 
boolean _Thread_queue_Extract_with_proxy(
  Thread_Control       *the_thread
);

/*
 *  _Thread_queue_First
 *
 *  DESCRIPTION:
 *
 *  This function returns a pointer to the "first" thread
 *  on the_thread_queue.  The "first" thread is selected
 *  based on the discipline of the_thread_queue.
 */

Thread_Control *_Thread_queue_First(
  Thread_queue_Control *the_thread_queue
);

/*
 *  _Thread_queue_Flush
 *
 *  DESCRIPTION:
 *
 *  This routine unblocks all threads blocked on the_thread_queue
 *  and cancels any associated timeouts.
 */

void _Thread_queue_Flush(
  Thread_queue_Control       *the_thread_queue,
  unsigned32                  status
);

/*
 *  _Thread_queue_Initialize
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the_thread_queue based on the
 *  discipline indicated in attribute_set.  The state set on
 *  threads which block on the_thread_queue is state.
 */

void _Thread_queue_Initialize(
  Thread_queue_Control         *the_thread_queue,
  Thread_queue_Disciplines      the_discipline,
  States_Control                state,
  unsigned32                    timeout_status
);

/*
 *  _Thread_queue_Dequeue_priority
 *
 *  DESCRIPTION:
 *
 *  This function returns a pointer to the highest priority
 *  thread waiting on the_thread_queue.  If no threads are waiting
 *  on the_thread_queue, then NULL is returned.
 */

Thread_Control *_Thread_queue_Dequeue_priority(
  Thread_queue_Control *the_thread_queue
);

/*
 *  _Thread_queue_Enqueue_priority
 *
 *  DESCRIPTION:
 *
 *  This routine enqueues the currently executing thread on
 *  the_thread_queue with an optional timeout using the
 *  priority discipline.
 */

void _Thread_queue_Enqueue_priority(
  Thread_queue_Control *the_thread_queue,
  Thread_Control       *the_thread,
  Watchdog_Interval     timeout
);

/*
 *  _Thread_queue_Extract_priority
 *
 *  DESCRIPTION:
 *
 *  This routine removes the_thread from the_thread_queue
 *  and cancels any timeouts associated with this blocking.
 */

void _Thread_queue_Extract_priority(
  Thread_queue_Control *the_thread_queue,
  Thread_Control       *the_thread
);

/*
 *  _Thread_queue_First_priority
 *
 *  DESCRIPTION:
 *
 *  This function returns a pointer to the "first" thread
 *  on the_thread_queue.  The "first" thread is the highest
 *  priority thread waiting on the_thread_queue.
 */

Thread_Control *_Thread_queue_First_priority(
  Thread_queue_Control *the_thread_queue
);

/*
 *  _Thread_queue_Dequeue_FIFO
 *
 *  DESCRIPTION:
 *
 *  This function returns a pointer to the thread which has
 *  been waiting the longest on  the_thread_queue.  If no
 *  threads are waiting on the_thread_queue, then NULL is returned.
 */

Thread_Control *_Thread_queue_Dequeue_fifo(
  Thread_queue_Control *the_thread_queue
);

/*
 *  _Thread_queue_Enqueue_FIFO
 *
 *  DESCRIPTION:
 *
 *  This routine enqueues the currently executing thread on
 *  the_thread_queue with an optional timeout using the
 *  FIFO discipline.
 */

void _Thread_queue_Enqueue_fifo(
  Thread_queue_Control *the_thread_queue,
  Thread_Control       *the_thread,
  Watchdog_Interval     timeout
);

/*
 *  _Thread_queue_Extract_FIFO
 *
 *  DESCRIPTION:
 *
 *  This routine removes the_thread from the_thread_queue
 *  and cancels any timeouts associated with this blocking.
 */

void _Thread_queue_Extract_fifo(
  Thread_queue_Control *the_thread_queue,
  Thread_Control       *the_thread
);

/*
 *  _Thread_queue_First_FIFO
 *
 *  DESCRIPTION:
 *
 *  This function returns a pointer to the "first" thread
 *  on the_thread_queue.  The first thread is the thread
 *  which has been waiting longest on the_thread_queue.
 */

Thread_Control *_Thread_queue_First_fifo(
  Thread_queue_Control *the_thread_queue
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/tqdata.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
