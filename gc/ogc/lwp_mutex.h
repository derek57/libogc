/*  mutex.h
 *
 *  This include file contains all the constants and structures associated
 *  with the Mutex Handler.  A mutex is an enhanced version of the standard
 *  Dijkstra binary semaphore used to provide synchronization and mutual 
 *  exclusion capabilities.
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

#ifndef __LWP_MUTEX_H__
#define __LWP_MUTEX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <lwp_threadq.h>

#include "lwp_config.h"

/**
 *  @brief The blocking disciplines for a mutex.
 *
 *  This enumerated type defines the blocking disciplines for a mutex.
 */

typedef enum {
  /** This specifies that threads will wait for the mutex in FIFO order. */
  CORE_MUTEX_DISCIPLINES_FIFO,
  /** This specifies that threads will wait for the mutex in priority order.  */
  CORE_MUTEX_DISCIPLINES_PRIORITY,
  /** This specifies that threads will wait for the mutex in priority order.
   *  Additionally, the Priority Inheritance Protocol will be in effect.
   */
  CORE_MUTEX_DISCIPLINES_PRIORITY_INHERIT,
  /** This specifies that threads will wait for the mutex in priority order.
   *  Additionally, the Priority Ceiling Protocol will be in effect.
   */
  CORE_MUTEX_DISCIPLINES_PRIORITY_CEILING
}   CORE_mutex_Disciplines;

/**
 *  @brief The possible Mutex handler return statuses.
 *
 *  This enumerated type defines the possible Mutex handler return statuses.
 */
typedef enum {
  /** This status indicates that the operation completed successfully. */
  CORE_MUTEX_STATUS_SUCCESSFUL,
  /** This status indicates that the calling task did not want to block
   *  and the operation was unable to complete immediately because the
   *  resource was unavailable.
   */
  CORE_MUTEX_STATUS_UNSATISFIED_NOWAIT,
#if defined(RTEMS_POSIX_API)
  /** This status indicates that an attempt was made to relock a mutex
   *  for which nesting is not configured.
   */
  CORE_MUTEX_STATUS_NESTING_NOT_ALLOWED,
#endif
  /** This status indicates that an attempt was made to release a mutex
   *  by a thread other than the thread which locked it.
   */
  CORE_MUTEX_STATUS_NOT_OWNER_OF_RESOURCE,
  /** This status indicates that the thread was blocked waiting for an
   *  operation to complete and the mutex was deleted.
   */
  CORE_MUTEX_WAS_DELETED,
  /** This status indicates that the calling task was willing to block
   *  but the operation was unable to complete within the time allotted
   *  because the resource never became available.
   */
  CORE_MUTEX_TIMEOUT,

#if defined(__RTEMS_STRICT_ORDER_MUTEX__)
  /** This status indicates that a thread not release the mutex which has
   *  the priority inheritance property in a right order.
   */
  CORE_MUTEX_RELEASE_NOT_ORDER,
#endif

  /** This status indicates that a thread of logically greater importance
   *  than the ceiling priority attempted to lock this mutex.
   */
  CORE_MUTEX_STATUS_CEILING_VIOLATED

}   CORE_mutex_Status;

/*
 *  Mutex lock nesting behavior
 *
 *  CORE_MUTEX_NESTING_ACQUIRES:
 *    This sequence has no blocking or errors:
 *         lock(m)
 *         lock(m)
 *         unlock(m)
 *         unlock(m)
 *
 *  CORE_MUTEX_NESTING_IS_ERROR
 *    This sequence returns an error at the indicated point:
 *        lock(m)
 *        lock(m)   - already locked error
 *        unlock(m)
 *
 *  CORE_MUTEX_NESTING_BLOCKS
 *    This sequence performs as indicated:
 *        lock(m)
 *        lock(m)   - deadlocks or timeouts
 *        unlock(m) - releases
 */

typedef enum {
  /**
   *    This sequence has no blocking or errors:
   *
   *         + lock(m)
   *         + lock(m)
   *         + unlock(m)
   *         + unlock(m)
   */
  CORE_MUTEX_NESTING_ACQUIRES,
#if defined(RTEMS_POSIX_API)
  /**
   *    This sequence returns an error at the indicated point:
   *
   *        + lock(m)
   *        + lock(m)   - already locked error
   *        + unlock(m)
   */
  CORE_MUTEX_NESTING_IS_ERROR,
#endif
  /**
   *    This sequence performs as indicated:
   *        + lock(m)
   *        + lock(m)   - deadlocks or timeouts
   *        + unlock(m) - releases
   */
  CORE_MUTEX_NESTING_BLOCKS
}  CORE_mutex_Nesting_behaviors;
 
/*
 *  Locked and unlocked values
 */

#define CORE_MUTEX_UNLOCKED 1
#define CORE_MUTEX_LOCKED   0

/**
 *  @brief The control block used to manage attributes of each mutex.
 *
 *  The following defines the control block used to manage the
 *  attributes of each mutex.
 */
typedef struct {
  /** This field indicates whether threads waiting on the mutex block in
   *  FIFO or priority order.
   */
  CORE_mutex_Disciplines       discipline;
  /** This field determines what the behavior of this mutex instance will
   *  be when attempting to acquire the mutex when it is already locked.
   */
  CORE_mutex_Nesting_behaviors lock_nesting_behavior;
  /** This field contains the ceiling priority to be used if that protocol
   *  is selected.
   */
  unsigned8                    priority_ceiling;
  /** When this field is true, then only the thread that locked the mutex
   *  is allowed to unlock it.
   */
  bool                         only_owner_release;
}   CORE_mutex_Attributes;

/**
 *  @brief Control block used to manage each mutex.
 *
 *  The following defines the control block used to manage each mutex.
 */
typedef struct {
  /** This field is the Waiting Queue used to manage the set of tasks
   *  which are blocked waiting to lock the mutex.
   */
  Thread_queue_Control    Wait_queue;
  /** This element is the set of attributes which define this instance's
   *  behavior.
   */
  CORE_mutex_Attributes   Attributes;
  /** This element contains the current state of the mutex.
   */
  unsigned32              lock;
  /** This element contains the number of times the mutex has been acquired
   *  nested.  This must be zero (0) before the mutex is actually unlocked.
   */
  unsigned32              nest_count;
  /** This is the number of waiting threads. */
  unsigned32              blocked_count;
  /** This element points to the thread which is currently holding this mutex.
   *  The holder is the last thread to successfully lock the mutex and which
   *  has not unlocked it.  If the thread is not locked, there is no holder.
   */
  Thread_Control         *holder;
#ifdef __RTEMS_STRICT_ORDER_MUTEX__
  /** This field is used to manipulate the priority inheritance mutex queue*/
  CORE_mutex_order_list   queue;
#endif

}   CORE_mutex_Control;

/*
 *  _CORE_mutex_Initialize
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the mutex based on the parameters passed.
 */

void _CORE_mutex_Initialize(CORE_mutex_Control *mutex,CORE_mutex_Attributes *attrs,unsigned32 init_lock);

/*
 *  _CORE_mutex_Seize
 *
 *  DESCRIPTION:
 *
 *  This routine attempts to receive a unit from the_mutex.
 *  If a unit is available or if the wait flag is FALSE, then the routine
 *  returns.  Otherwise, the calling task is blocked until a unit becomes
 *  available.
 *
 *  NOTE:  For performance reasons, this routine is implemented as
 *         a macro that uses two support routines.
 */

RTEMS_INLINE_ROUTINE int _CORE_mutex_Seize_interrupt_trylock(
  CORE_mutex_Control  *the_mutex,
  ISR_Level           *level_p
);

void _CORE_mutex_Seize_interrupt_blocking(
  CORE_mutex_Control  *the_mutex,
  Watchdog_Interval    timeout
);

#define _CORE_mutex_Seize( \
  _the_mutex, _id, _wait, _timeout, _level ) \
  do { \
    if ( _CORE_mutex_Seize_interrupt_trylock( _the_mutex, &_level ) ) {  \
      if ( !_wait ) { \
        _ISR_Enable( _level ); \
        _Thread_Executing->Wait.return_code = \
          CORE_MUTEX_STATUS_UNSATISFIED_NOWAIT; \
      } else { \
        _Thread_queue_Enter_critical_section( &(_the_mutex)->Wait_queue ); \
        _Thread_Executing->Wait.queue = &(_the_mutex)->Wait_queue; \
        _Thread_Executing->Wait.id    = _id; \
        _Thread_Disable_dispatch(); \
        _ISR_Enable( _level ); \
       _CORE_mutex_Seize_interrupt_blocking( _the_mutex, _timeout ); \
      } \
    } \
  } while (0)

/*
 *  _CORE_mutex_Surrender
 *
 *  DESCRIPTION:
 *
 *  This routine frees a unit to the mutex.  If a task was blocked waiting for
 *  a unit from this mutex, then that task will be readied and the unit
 *  given to that task.  Otherwise, the unit will be returned to the mutex.
 */

CORE_mutex_Status _CORE_mutex_Surrender(
  CORE_mutex_Control                *the_mutex
);

/*
 *  _CORE_mutex_Flush
 *
 *  DESCRIPTION:
 *
 *  This routine assists in the deletion of a mutex by flushing the associated
 *  wait queue.
 */
 
void _CORE_mutex_Flush(
  CORE_mutex_Control         *the_mutex,
  unsigned32                  status
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_mutex.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
