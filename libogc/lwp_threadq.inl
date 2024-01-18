/*  tqdata.inl
 *
 *  This file contains the RTEMS_INLINE_ROUTINE implementation of the inlined
 *  routines needed to support the Thread Queue Data.
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

#ifndef __LWP_THREADQ_INL__
#define __LWP_THREADQ_INL__

/*PAGE
 *
 *  _Thread_queue_Enter_critical_section
 *
 *  DESCRIPTION:
 *
 *  This routine is invoked to indicate that the specified thread queue is
 *  entering a critical section.
 */
 
RTEMS_INLINE_ROUTINE void _Thread_queue_Enter_critical_section (
  Thread_queue_Control *the_thread_queue
)
{
  the_thread_queue->sync_state = THREAD_QUEUE_NOTHING_HAPPENED;
}

#endif
