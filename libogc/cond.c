/*-------------------------------------------------------------

cond.c -- Thread subsystem V

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/


#include <stdlib.h>
#include <errno.h>
#include "asm.h"
#include "mutex.h"
#include "lwp_threadq.h"
#include "lwp_objmgr.h"
#include "lwp_config.h"
#include "cond.h"

#define LWP_CHECK_COND( hndl )  \
{                               \
  if ( ( ( hndl ) == LWP_COND_NULL ) || ( _Objects_Get_node( hndl ) != OBJECTS_POSIX_CONDITION_VARIABLES ) )    \
    return NULL;                \
}

/*
 *  Data Structure used to manage a POSIX condition variable
 */

typedef struct {
   Objects_Control Object;
   pthread_mutex_t Mutex;
   Thread_queue_Control Wait_queue;
}  POSIX_Condition_variables_Control;

/*
 *  The following defines the information control block used to manage
 *  this class of objects.
 */

Objects_Information _POSIX_Condition_variables_Information;

extern int clock_gettime(struct timespec *tp);
extern void timespec_subtract(const struct timespec *tp_start,const struct timespec *tp_end,struct timespec *result);

/*PAGE
 *
 *  _POSIX_Condition_variables_Manager_initialization
 *
 *  This routine initializes all condition variable manager related data 
 *  structures.
 *
 *  Input parameters:
 *    maximum_condition_variables - maximum configured condition_variables
 *
 *  Output parameters:  NONE
 */

void _POSIX_Condition_variables_Manager_initialization( void )
{
  _Objects_Initialize_information(
    &_POSIX_Condition_variables_Information,
    CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES,
    sizeof( POSIX_Condition_variables_Control )
  );
}

/*PAGE
 *
 *  _POSIX_Condition_variables_Get
 *
 *  DESCRIPTION:
 *
 *  This function maps condition variable IDs to condition variable control 
 *  blocks.  If ID corresponds to a local condition variable, then it returns
 *  the_condition variable control pointer which maps to ID and location
 *  is set to OBJECTS_LOCAL.  if the condition variable ID is global and
 *  resides on a remote node, then location is set to OBJECTS_REMOTE,
 *  and the_condition variable is undefined.  Otherwise, location is set
 *  to OBJECTS_ERROR and the_condition variable is undefined.
 */

RTEMS_INLINE_ROUTINE POSIX_Condition_variables_Control
* _POSIX_Condition_variables_Get (
  pthread_cond_t cond
)
{
  /* XXX should support COND_INITIALIZER */ 
  LWP_CHECK_COND( cond );

  return ( POSIX_Condition_variables_Control *)_Objects_Get(
    &_POSIX_Condition_variables_Information,
    _Objects_Get_index( cond )
  );
}

/*PAGE
 *
 *  _POSIX_Condition_variables_Free
 *
 *  DESCRIPTION:
 *
 *  This routine frees a condition variable control block to the
 *  inactive chain of free condition variable control blocks.
 */

RTEMS_INLINE_ROUTINE void _POSIX_Condition_variables_Free(
  POSIX_Condition_variables_Control *the_condition_variable
)
{
  _Objects_Close(
    &_POSIX_Condition_variables_Information,
    &the_condition_variable->Object
  );

  _Objects_Free(
    &_POSIX_Condition_variables_Information,
    &the_condition_variable->Object
  );
}

/*PAGE
 *
 *  _POSIX_Condition_variables_Allocate
 *
 *  DESCRIPTION:
 *
 *  This function allocates a condition variable control block from
 *  the inactive chain of free condition variable control blocks.
 */

STATIC POSIX_Condition_variables_Control
  *_POSIX_Condition_variables_Allocate( void )
{
  POSIX_Condition_variables_Control *the_cond;

  _Thread_Disable_dispatch();

  the_cond = (POSIX_Condition_variables_Control *)_Objects_Allocate( &_POSIX_Condition_variables_Information );

  if ( the_cond ) {
    _Objects_Open( &_POSIX_Condition_variables_Information, &the_cond->Object );
    return the_cond;
  }

  _Thread_Enable_dispatch();

  return NULL;
}

/*PAGE
 *
 *  _POSIX_Condition_variables_Wait_support
 *
 *  DESCRIPTION:
 *
 *  A support routine which implements guts of the blocking, non-blocking, and
 *  timed wait version of condition variable wait routines.
 */

STATIC int _POSIX_Condition_variables_Wait_support(
  pthread_cond_t             cond,
  pthread_mutex_t            mutex,
  Watchdog_Interval          timeout,
  unsigned8                  already_timedout
)
{
  unsigned32                         status;
  unsigned32                         mutex_status;
  ISR_Level                          level;
  POSIX_Condition_variables_Control *the_cond;

  the_cond = _POSIX_Condition_variables_Get( cond );

  if ( !the_cond )
    return -1;

  if ( the_cond->Mutex != POSIX_CONDITION_VARIABLES_NO_MUTEX && the_cond->Mutex != mutex) {
    _Thread_Enable_dispatch();
    return EINVAL;
  }

  pthread_mutex_unlock( mutex );
/* XXX ignore this for now  since behavior is undefined
  if ( mutex_status ) {
    _Thread_Enable_dispatch();
    return EINVAL;
  }
*/

  if ( !already_timedout ) {
    the_cond->Mutex = mutex;

    _ISR_Disable( level );
    _Thread_queue_Enter_critical_section( &the_cond->Wait_queue );
    _Thread_Executing->Wait.return_code = 0;
    _Thread_Executing->Wait.queue       = &the_cond->Wait_queue;
    _Thread_Executing->Wait.id          = cond;
    _ISR_Enable( level );

    _Thread_queue_Enqueue( &the_cond->Wait_queue, timeout );

    _Thread_Enable_dispatch();

    /*
     *  Switch ourself out because we blocked as a result of the 
     *  _Thread_queue_Enqueue.
     */

    status = _Thread_Executing->Wait.return_code;
    if ( status && status != ETIMEDOUT )
      return status;

  } else {
    _Thread_Enable_dispatch();
    status = ETIMEDOUT;
  }

  /*
   *  When we get here the dispatch disable level is 0.
   */

  mutex_status = pthread_mutex_lock( mutex );
  if ( mutex_status )
    return EINVAL;

  return status;
}

/*PAGE
 *
 *  _POSIX_Condition_variables_Signal_support
 *
 *  DESCRIPTION:
 *
 *  A support routine which implements guts of the broadcast and single task
 *  wake up version of the "signal" operation.
 */

STATIC int _POSIX_Condition_variables_Signal_support(
  pthread_cond_t             cond,
  unsigned8                  is_broadcast
)
{
  Thread_Control                    *the_thread;
  POSIX_Condition_variables_Control *the_cond;

  the_cond = _POSIX_Condition_variables_Get( cond );

  if ( !the_cond )
    return -1;

  do {
    the_thread = _Thread_queue_Dequeue( &the_cond->Wait_queue );
    if ( !the_thread )
      the_cond->Mutex = POSIX_CONDITION_VARIABLES_NO_MUTEX;
   } while ( is_broadcast && the_thread );

  _Thread_Enable_dispatch();

  return 0;
}

/*PAGE
 *
 *  pthread_cond_init
 *
 *  11.4.2 Initializing and Destroying a Condition Variable, 
 *         P1003.1c/Draft 10, p. 87
 */

int pthread_cond_init(
  pthread_cond_t           *cond
)
{
  POSIX_Condition_variables_Control   *the_cond;

  if ( !cond )
    return -1;

  the_cond = _POSIX_Condition_variables_Allocate();

  if ( !the_cond )
    return ENOMEM;

  the_cond->Mutex = POSIX_CONDITION_VARIABLES_NO_MUTEX;

/* XXX some more initialization might need to go here */
  _Thread_queue_Initialize(
    &the_cond->Wait_queue,
    THREAD_QUEUE_DISCIPLINE_FIFO,
    STATES_WAITING_FOR_CONDITION_VARIABLE,
    ETIMEDOUT
  );

  *cond = (pthread_cond_t)_Objects_Build_id( OBJECTS_POSIX_CONDITION_VARIABLES, _Objects_Get_index( the_cond->Object.id ) );

  _Thread_Enable_dispatch();

  return 0;
}

/*PAGE
 *
 *  pthread_cond_wait
 *
 *  11.4.4 Waiting on a Condition, P1003.1c/Draft 10, p. 105
 */

int pthread_cond_wait(
  pthread_cond_t           cond,
  pthread_mutex_t          mutex
)
{
  return _POSIX_Condition_variables_Wait_support(
    cond,
    mutex,
    RTEMS_NO_TIMEOUT,
    FALSE
  );
}

/*PAGE
 *
 *  pthread_cond_signal
 *
 *  11.4.3 Broadcasting and Signaling a Condition, P1003.1c/Draft 10, p. 101
 */

int pthread_cond_signal(
  pthread_cond_t         cond
)
{
  return _POSIX_Condition_variables_Signal_support( cond, FALSE );
}

/*PAGE
 *
 *  pthread_cond_broadcast
 *
 *  11.4.3 Broadcasting and Signaling a Condition, P1003.1c/Draft 10, p. 101
 */

int pthread_cond_broadcast(
  pthread_cond_t         cond
)
{
  return _POSIX_Condition_variables_Signal_support( cond, TRUE );
}

/*PAGE
 *
 *  pthread_cond_timedwait
 *
 *  11.4.4 Waiting on a Condition, P1003.1c/Draft 10, p. 105
 */

int pthread_cond_timedwait(
  pthread_cond_t              cond,
  pthread_mutex_t             mutex,
  const struct timespec      *abstime
)
{
  Watchdog_Interval timeout = RTEMS_NO_TIMEOUT;
  u8 already_timedout       = 0;

  if ( abstime )
    timeout = _POSIX_Timespec_to_interval( abstime );

  return _POSIX_Condition_variables_Wait_support(
    cond,
    mutex,
    timeout,
    already_timedout
  );
}

/*PAGE
 *
 *  pthread_cond_destroy
 *
 *  11.4.2 Initializing and Destroying a Condition Variable, 
 *         P1003.1c/Draft 10, p. 87
 */

int pthread_cond_destroy(
  pthread_cond_t            cond
)
{
  POSIX_Condition_variables_Control *the_cond;

  the_cond = _POSIX_Condition_variables_Get( cond );

  if ( !the_cond )
    return -1;

  if ( _Thread_queue_First( &the_cond->Wait_queue ) ) {
    _Thread_Enable_dispatch();
    return EBUSY;
  }

  _Thread_Enable_dispatch();

  _POSIX_Condition_variables_Free( the_cond );

  return 0;
}
