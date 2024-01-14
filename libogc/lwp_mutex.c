/*
 *  Mutex Handler
 *
 *  DESCRIPTION:
 *
 *  This package is the implementation of the Mutex Handler.
 *  This handler provides synchronization and mutual exclusion capabilities.
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

#include "asm.h"
#include "lwp_mutex.h"

/*PAGE
 *
 *  _CORE_mutex_Initialize
 *
 *  This routine initializes a mutex at create time and set the control
 *  structure according to the values passed.
 *
 *  Input parameters: 
 *    the_mutex             - the mutex control block to initialize
 *    the_mutex_attributes  - the mutex attributes specified at create time
 *    initial_lock          - mutex initial lock or unlocked status
 *
 *  Output parameters:  NONE
 */

void _CORE_mutex_Initialize(
  CORE_mutex_Control           *the_mutex,
  CORE_mutex_Attributes        *the_mutex_attributes,
  unsigned32                    initial_lock
)
{

/* Add this to the RTEMS environment later ????????? 
  rtems_assert( initial_lock == CORE_MUTEX_LOCKED ||
                initial_lock == CORE_MUTEX_UNLOCKED );
 */

  the_mutex->Attributes    = *the_mutex_attributes;
  the_mutex->lock          = initial_lock;
  the_mutex->blocked_count = 0;

#if 0
  if ( !the_mutex_attributes->only_owner_release &&
       the_mutex_attributes->nesting_allowed ) {
    _Internal_error_Occurred(
      INTERNAL_ERROR_CORE,
      TRUE,
      INTERNAL_ERROR_BAD_ATTRIBUTES
    );
  }
#endif

  if ( initial_lock == CORE_MUTEX_LOCKED ) {
    the_mutex->nest_count = 1;
    the_mutex->holder     = _Thread_Executing;
    if ( _CORE_mutex_Is_inherit_priority( the_mutex_attributes ) ||
         _CORE_mutex_Is_priority_ceiling( the_mutex_attributes ) )
      _Thread_Executing->resource_count++;
  } else {
    the_mutex->nest_count = 0;
    the_mutex->holder     = NULL;
  }

  _Thread_queue_Initialize(
    &the_mutex->Wait_queue,
    _CORE_mutex_Is_fifo( the_mutex_attributes ) ?
      THREAD_QUEUE_DISCIPLINE_FIFO : THREAD_QUEUE_DISCIPLINE_PRIORITY,
    STATES_WAITING_FOR_MUTEX,
    CORE_MUTEX_TIMEOUT
  );
}

/*
 *  _CORE_mutex_Surrender
 *
 *  DESCRIPTION:
 *
 *  This routine frees a unit to the mutex.  If a task was blocked waiting for
 *  a unit from this mutex, then that task will be readied and the unit
 *  given to that task.  Otherwise, the unit will be returned to the mutex.
 *
 *  Input parameters:
 *    the_mutex            - the mutex to be flushed
 *
 *  Output parameters:
 *    CORE_MUTEX_STATUS_SUCCESSFUL - if successful
 *    core error code              - if unsuccessful
 */

CORE_mutex_Status _CORE_mutex_Surrender(
  CORE_mutex_Control                *the_mutex
)
{
  Thread_Control *the_thread;
  Thread_Control *holder;

  holder    = the_mutex->holder;

  /*
   *  The following code allows a thread (or ISR) other than the thread
   *  which acquired the mutex to release that mutex.  This is only
   *  allowed when the mutex in quetion is FIFO or simple Priority
   *  discipline.  But Priority Ceiling or Priority Inheritance mutexes
   *  must be released by the thread which acquired them.
   */ 

  if ( the_mutex->Attributes.only_owner_release ) {
    if ( !_Thread_Is_executing( holder ) )
      return CORE_MUTEX_STATUS_NOT_OWNER_OF_RESOURCE;
  }

  /* XXX already unlocked -- not right status */

  if ( !the_mutex->nest_count ) 
    return CORE_MUTEX_STATUS_SUCCESSFUL;

  the_mutex->nest_count--;

  if ( the_mutex->nest_count != 0 ) {
    switch ( the_mutex->Attributes.lock_nesting_behavior ) {
      case CORE_MUTEX_NESTING_ACQUIRES:
        return CORE_MUTEX_STATUS_SUCCESSFUL;
      case CORE_MUTEX_NESTING_IS_ERROR:
        /* should never occur */
        return CORE_MUTEX_STATUS_NESTING_NOT_ALLOWED;
      case CORE_MUTEX_NESTING_BLOCKS:
        /* Currently no API exercises this behavior. */
        break;
    }
  }

  if ( _CORE_mutex_Is_inherit_priority( &the_mutex->Attributes ) ||
       _CORE_mutex_Is_priority_ceiling( &the_mutex->Attributes ) )
    holder->resource_count--;
  the_mutex->holder    = NULL;

  /*
   *  Whether or not someone is waiting for the mutex, an
   *  inherited priority must be lowered if this is the last
   *  mutex (i.e. resource) this task has.
   */

  if ( _CORE_mutex_Is_inherit_priority( &the_mutex->Attributes ) ||
       _CORE_mutex_Is_priority_ceiling( &the_mutex->Attributes ) ) {
    if ( holder->resource_count == 0 && 
         holder->real_priority != holder->current_priority ) {
      _Thread_Change_priority( holder, holder->real_priority, TRUE );
    }
  }

  if ( ( the_thread = _Thread_queue_Dequeue( &the_mutex->Wait_queue ) ) ) {

#if defined(RTEMS_MULTIPROCESSING)
    if ( !_Objects_Is_local_id( the_thread->Object.id ) ) {
      
      the_mutex->holder     = NULL;
      the_mutex->holder_id  = the_thread->Object.id;
      the_mutex->nest_count = 1;

      ( *api_mutex_mp_support)( the_thread, id );

    } else 
#endif
    {

      the_mutex->nest_count = 1;
      the_mutex->holder     = the_thread;
      if ( _CORE_mutex_Is_inherit_priority( &the_mutex->Attributes ) ||
           _CORE_mutex_Is_priority_ceiling( &the_mutex->Attributes ) )
        the_thread->resource_count++;

     /*
      *  No special action for priority inheritance or priority ceiling
      *  because the_thread is guaranteed to be the highest priority
      *  thread waiting for the mutex.
      */
    }
  } else
    the_mutex->lock = CORE_MUTEX_UNLOCKED;

  return CORE_MUTEX_STATUS_SUCCESSFUL;
}

/*PAGE
 *
 *  _CORE_mutex_Seize (interrupt blocking support)
 *
 *  This routine blocks the caller thread after an attempt attempts to obtain
 *  the specified mutex has failed.
 *
 *  Input parameters:
 *    the_mutex - pointer to mutex control block
 *    timeout   - number of ticks to wait (0 means forever)
 */

void _CORE_mutex_Seize_interrupt_blocking(
  CORE_mutex_Control  *the_mutex,
  Watchdog_Interval    timeout
)
{
  Thread_Control   *executing;

  executing = _Thread_Executing;
  if ( _CORE_mutex_Is_inherit_priority( &the_mutex->Attributes ) ) {
    if ( the_mutex->holder->current_priority > executing->current_priority ) {
      _Thread_Change_priority(
        the_mutex->holder,
        executing->current_priority,
        FALSE
      );
    }
  }

  the_mutex->blocked_count++;
  _Thread_queue_Enqueue( &the_mutex->Wait_queue, timeout );

  if ( _Thread_Executing->Wait.return_code == CORE_MUTEX_STATUS_SUCCESSFUL ) {
    /*
     *  if CORE_MUTEX_DISCIPLINES_PRIORITY_INHERIT then nothing to do
     *  because this task is already the highest priority.
     */

    if ( _CORE_mutex_Is_priority_ceiling( &the_mutex->Attributes ) ) {
      if (the_mutex->Attributes.priority_ceiling < executing->current_priority){
        _Thread_Change_priority(
          executing,
          the_mutex->Attributes.priority_ceiling,
          FALSE
        );
      }
    }
  }
  _Thread_Enable_dispatch();
}

/*PAGE
 *
 *  _CORE_mutex_Flush
 *
 *  This function a flushes the mutex's task wait queue.
 *
 *  Input parameters:
 *    the_mutex              - the mutex to be flushed
 *    status                 - status to pass to thread
 *
 *  Output parameters:  NONE
 */

void _CORE_mutex_Flush(
  CORE_mutex_Control         *the_mutex,
  unsigned32                  status
)
{
  _Thread_queue_Flush(
    &the_mutex->Wait_queue,
    status
  );
}
