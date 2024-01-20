/*  core_sem.h
 *
 *  This include file contains all the constants and structures associated
 *  with the Counting Semaphore Handler.  A counting semaphore is the
 *  standard Dijkstra binary semaphore used to provide synchronization
 *  and mutual exclusion capabilities.
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

#ifndef __LWP_SEMA_H__
#define __LWP_SEMA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <threadq.h>

/*
 *  Blocking disciplines for a semaphore.
 */

typedef enum {
  /** This specifies that threads will wait for the semaphore in FIFO order. */
  CORE_SEMAPHORE_DISCIPLINES_FIFO,
  /** This specifies that threads will wait for the semaphore in
   *  priority order.
   */
  CORE_SEMAPHORE_DISCIPLINES_PRIORITY
}   CORE_semaphore_Disciplines;

/*
 *  Core Semaphore handler return statuses.
 */

typedef enum {
  /** This status indicates that the operation completed successfully. */
  CORE_SEMAPHORE_STATUS_SUCCESSFUL,
  /** This status indicates that the calling task did not want to block
   *  and the operation was unable to complete immediately because the
   *  resource was unavailable.
   */
  CORE_SEMAPHORE_STATUS_UNSATISFIED_NOWAIT,
  /** This status indicates that the thread was blocked waiting for an
   *  operation to complete and the semaphore was deleted.
   */
  CORE_SEMAPHORE_WAS_DELETED,
  /** This status indicates that the calling task was willing to block
   *  but the operation was unable to complete within the time allotted
   *  because the resource never became available.
   */
  CORE_SEMAPHORE_TIMEOUT,
  /** This status indicates that an attempt was made to unlock the semaphore
   *  and this would have made its count greater than that allowed.
   */
  CORE_SEMAPHORE_MAXIMUM_COUNT_EXCEEDED
}   CORE_semaphore_Status;

/*
 *  The following defines the control block used to manage the 
 *  attributes of each semaphore.
 */

typedef struct {
  /** This element indicates the maximum count this semaphore may have. */
  unsigned32                  maximum_count;
  /** This field indicates whether threads waiting on the semaphore block in
   *  FIFO or priority order.
   */
  CORE_semaphore_Disciplines  discipline;
}   CORE_semaphore_Attributes;
 
/*
 *  The following defines the control block used to manage each 
 *  counting semaphore.
 */
 
typedef struct {
  /** This field is the Waiting Queue used to manage the set of tasks
   *  which are blocked waiting to obtain the semaphore.
   */
  Thread_queue_Control        Wait_queue;
  /** This element is the set of attributes which define this instance's
   *  behavior.
   */
  CORE_semaphore_Attributes   Attributes;
  /** This element contains the current count of this semaphore. */
  unsigned32                  count;
}   CORE_semaphore_Control;

/*
 *  _CORE_semaphore_Initialize
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the semaphore based on the parameters passed.
 */

void _CORE_semaphore_Initialize(
  CORE_semaphore_Control       *the_semaphore,
  CORE_semaphore_Attributes    *the_semaphore_attributes,
  unsigned32                    initial_value
);
 
/*
 *  _CORE_semaphore_Seize
 *
 *  DESCRIPTION:
 *
 *  This routine attempts to receive a unit from the_semaphore.
 *  If a unit is available or if the wait flag is FALSE, then the routine
 *  returns.  Otherwise, the calling task is blocked until a unit becomes
 *  available.
 */

CORE_semaphore_Status _CORE_semaphore_Seize(
  CORE_semaphore_Control  *the_semaphore,
  Objects_Id               id,
  boolean                  wait,
  Watchdog_Interval        timeout
);
 
/*
 *  _CORE_semaphore_Surrender
 *
 *  DESCRIPTION:
 *
 *  This routine frees a unit to the semaphore.  If a task was blocked waiting
 *  for a unit from this semaphore, then that task will be readied and the unit
 *  given to that task.  Otherwise, the unit will be returned to the semaphore.
 */

CORE_semaphore_Status _CORE_semaphore_Surrender(
  CORE_semaphore_Control                *the_semaphore,
  Objects_Id                             id
);
 
/*
 *  _CORE_semaphore_Flush
 *
 *  DESCRIPTION:
 *
 *  This routine assists in the deletion of a semaphore by flushing the
 *  associated wait queue.
 */

void _CORE_semaphore_Flush(
  CORE_semaphore_Control         *the_semaphore,
  unsigned32                      status
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_sema.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
