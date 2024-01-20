/*
 *  CORE Semaphore Handler
 *
 *  DESCRIPTION:
 *
 *  This package is the implementation of the CORE Semaphore Handler.
 *  This core object utilizes standard Dijkstra counting semaphores to provide
 *  synchronization and mutual exclusion capabilities.
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
#include "coresem.h"

/*PAGE
 *
 *  CORE_semaphore_Initialize
 *
 *  This function initialize a semaphore and sets the initial value based
 *  on the given count.
 *
 *  Input parameters:
 *    the_semaphore            - the semaphore control block to initialize
 *    the_semaphore_attributes - the attributes specified at create time
 *    initial_value            - semaphore's initial value
 *
 *  Output parameters:  NONE
 */

void _CORE_semaphore_Initialize(
  CORE_semaphore_Control       *the_semaphore,
  CORE_semaphore_Attributes    *the_semaphore_attributes,
  unsigned32                    initial_value
)
{

  the_semaphore->Attributes = *the_semaphore_attributes;
  the_semaphore->count      = initial_value;

  _Thread_queue_Initialize(
    &the_semaphore->Wait_queue,
    _CORE_semaphore_Is_priority( the_semaphore_attributes ) ?
              THREAD_QUEUE_DISCIPLINE_PRIORITY : THREAD_QUEUE_DISCIPLINE_FIFO,
    STATES_WAITING_FOR_SEMAPHORE,
    CORE_SEMAPHORE_TIMEOUT
  );
}

/*PAGE
 *
 *  _CORE_semaphore_Surrender
 *
 *  Input parameters:
 *    the_semaphore            - the semaphore to be flushed
 *    id                       - id of parent semaphore
 *
 *  Output parameters:
 *    CORE_SEMAPHORE_STATUS_SUCCESSFUL - if successful
 *    core error code                  - if unsuccessful
 *
 *  Output parameters:
 */

CORE_semaphore_Status _CORE_semaphore_Surrender(
  CORE_semaphore_Control                *the_semaphore,
  Objects_Id                             id
)
{
  Thread_Control *the_thread;
  ISR_Level       level;
  CORE_semaphore_Status status;

  status = CORE_SEMAPHORE_STATUS_SUCCESSFUL;

  if((the_thread=_Thread_queue_Dequeue(&the_semaphore->Wait_queue)))
    return status;
  else {
    _ISR_Disable( level );
      if ( the_semaphore->count <= the_semaphore->Attributes.maximum_count )
        the_semaphore->count += 1;
      else
        status = CORE_SEMAPHORE_MAXIMUM_COUNT_EXCEEDED;
    _ISR_Enable( level );
  }

  return status;
}

/*PAGE
 *
 *  _CORE_semaphore_Seize
 *
 *  This routine attempts to allocate a core semaphore to the calling thread.
 *
 *  Input parameters:
 *    the_semaphore - pointer to semaphore control block
 *    id            - id of object to wait on
 *    wait          - TRUE if wait is allowed, FALSE otherwise
 *    timeout       - number of ticks to wait (0 means forever)
 *
 *  Output parameters:
 *    CORE_SEMAPHORE_STATUS_SUCCESSFUL - if successful
 *
 *  INTERRUPT LATENCY:
 *    available
 *    wait
 */

CORE_semaphore_Status _CORE_semaphore_Seize(
  CORE_semaphore_Control  *the_semaphore,
  Objects_Id               id,
  boolean                  wait,
  Watchdog_Interval        timeout
)
{
  Thread_Control *executing;
  ISR_Level       level;

  executing = _Thread_Executing;
  executing->Wait.return_code = CORE_SEMAPHORE_STATUS_SUCCESSFUL;
  _ISR_Disable( level );
  if ( the_semaphore->count != 0 ) {
    the_semaphore->count -= 1;
    _ISR_Enable( level );
    return CORE_SEMAPHORE_STATUS_SUCCESSFUL;
  }

  if ( !wait ) {
    _ISR_Enable( level );
    executing->Wait.return_code = CORE_SEMAPHORE_STATUS_UNSATISFIED_NOWAIT;
    return CORE_SEMAPHORE_STATUS_UNSATISFIED_NOWAIT;
  }

  _Thread_queue_Enter_critical_section( &the_semaphore->Wait_queue );
  executing->Wait.queue          = &the_semaphore->Wait_queue;
  executing->Wait.id             = id;
  _ISR_Enable( level );

  _Thread_queue_Enqueue( &the_semaphore->Wait_queue, timeout );
  return CORE_SEMAPHORE_STATUS_SUCCESSFUL;
}

/*PAGE
 *
 *  _CORE_semaphore_Flush
 *
 *  This function a flushes the semaphore's task wait queue.
 *
 *  Input parameters:
 *    the_semaphore          - the semaphore to be flushed
 *    status                 - status to pass to thread
 *
 *  Output parameters:  NONE
 */
 
void _CORE_semaphore_Flush(
  CORE_semaphore_Control     *the_semaphore,
  unsigned32                  status
)
{
 
  _Thread_queue_Flush(
    &the_semaphore->Wait_queue,
    status
  );
 
}
