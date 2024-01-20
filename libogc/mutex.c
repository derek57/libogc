/*-------------------------------------------------------------

mutex.c -- Thread subsystem III

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
#include "coremutex.h"
#include "lwp_objmgr.h"
#include "lwp_config.h"
#include "mutex.h"

#define LWP_CHECK_MUTEX( hndl ) \
{                               \
  if ( ( ( hndl ) == POSIX_CONDITION_VARIABLES_NO_MUTEX ) || ( _Objects_Get_node( hndl ) != OBJECTS_POSIX_MUTEXES ) )  \
    return NULL;                \
}

/*
 *  Data Structure used to manage a POSIX mutex
 */

typedef struct
{
	Objects_Control Object;
	CORE_mutex_Control Mutex;
} POSIX_Mutex_Control;

/*
 *  The following defines the information control block used to manage
 *  this class of objects.
 */

Objects_Information _POSIX_Mutex_Information;

/*PAGE
 *
 *  _POSIX_Mutex_Lock_support
 *
 *  A support routine which implements guts of the blocking, non-blocking, and
 *  timed wait version of mutex lock.
 */

static int _POSIX_Mutex_Lock_support(
  pthread_mutex_t                  mutex,
  unsigned32                       timeout,
  unsigned8                        blocking
)
{
  ISR_Level            level;
  POSIX_Mutex_Control *the_mutex;

  if ( mutex == POSIX_CONDITION_VARIABLES_NO_MUTEX || _Objects_Get_node( mutex ) != OBJECTS_POSIX_MUTEXES )
    rtems_set_errno_and_return_minus_one( EINVAL );

  the_mutex = (POSIX_Mutex_Control *)_Objects_Get_isr_disable( &_POSIX_Mutex_Information, _Objects_Get_index( mutex ), &level );

  if ( !the_mutex )
    rtems_set_errno_and_return_minus_one( EINVAL );

  _CORE_mutex_Seize(
    &the_mutex->Mutex,
    the_mutex->Object.id,
    blocking,
    timeout,
    level
  );

  return _Thread_Executing->Wait.return_code;
}

/*PAGE
 *
 *  _POSIX_Mutex_Manager_initialization
 *
 *  This routine initializes all mutex manager related data structures.
 *
 *  Input parameters:
 *    maximum_mutexes - maximum configured mutexes
 *
 *  Output parameters:  NONE
 */

void _POSIX_Mutex_Manager_initialization( void )
{
  _Objects_Initialize_information(
    &_POSIX_Mutex_Information,
    CONFIGURE_MAXIMUM_POSIX_MUTEXES,
    sizeof( POSIX_Mutex_Control )
  );
}

/*
 *  _POSIX_Mutex_Get
 *
 *  DESCRIPTION:
 *
 *  This function maps mutexes IDs to mutexes control blocks.
 *  If ID corresponds to a local mutexes, then it returns
 *  the_mutex control pointer which maps to ID and location
 *  is set to OBJECTS_LOCAL.  if the mutexes ID is global and
 *  resides on a remote node, then location is set to OBJECTS_REMOTE,
 *  and the_mutex is undefined.  Otherwise, location is set
 *  to OBJECTS_ERROR and the_mutex is undefined.
 */

RTEMS_INLINE_ROUTINE POSIX_Mutex_Control *_POSIX_Mutex_Get(
  pthread_mutex_t lock
)
{
  LWP_CHECK_MUTEX(lock);
  return (POSIX_Mutex_Control *)_Objects_Get( &_POSIX_Mutex_Information, _Objects_Get_index( lock ) );
}

/*
 *  _POSIX_Mutex_Free
 *
 *  DESCRIPTION:
 *
 *  This routine frees a mutexes control block to the
 *  inactive chain of free mutexes control blocks.
 */

RTEMS_INLINE_ROUTINE void _POSIX_Mutex_Free(POSIX_Mutex_Control *the_mutex)
{
  _Objects_Close( &_POSIX_Mutex_Information, &the_mutex->Object );
  _Objects_Free( &_POSIX_Mutex_Information, &the_mutex->Object );
}

/*
 *  _POSIX_Mutex_Allocate
 *
 *  DESCRIPTION:
 *
 *  This function allocates a mutexes control block from
 *  the inactive chain of free mutexes control blocks.
 */

static POSIX_Mutex_Control *_POSIX_Mutex_Allocate( void )
{
  POSIX_Mutex_Control *the_mutex;

  _Thread_Disable_dispatch();
  the_mutex = (POSIX_Mutex_Control *)_Objects_Allocate( &_POSIX_Mutex_Information );

  if ( the_mutex ) {
    _Objects_Open( &_POSIX_Mutex_Information, &the_mutex->Object );
    return the_mutex;
  }

  _Thread_Unnest_dispatch();

  return NULL;
}

/*PAGE
 *
 *  11.3.2 Initializing and Destroying a Mutex, P1003.1c/Draft 10, p. 87
 *
 *  NOTE:  XXX Could be optimized so all the attribute error checking
 *             is not performed when attr is NULL.
 */

int pthread_mutex_init(
  pthread_mutex_t *mutex,
  bool             use_recursive
)
{
  CORE_mutex_Attributes  attr;
  POSIX_Mutex_Control   *the_mutex;

  /* Check for NULL mutex */

  if ( !mutex )
    rtems_set_errno_and_return_minus_one( EINVAL );

  the_mutex = _POSIX_Mutex_Allocate();

  if ( !the_mutex )
    rtems_set_errno_and_return_minus_one( EAGAIN );

  attr.discipline = CORE_MUTEX_DISCIPLINES_FIFO;
  attr.lock_nesting_behavior = use_recursive ? CORE_MUTEX_NESTING_ACQUIRES : CORE_MUTEX_NESTING_IS_ERROR;
  attr.only_owner_release = TRUE;
  attr.priority_ceiling = 1; //_POSIX_Priority_To_core( PRIORITY_MAXIMUM - 1 );

  /*
   *  Must be initialized to unlocked.
   */

  _CORE_mutex_Initialize(
    &the_mutex->Mutex,
    &attr,
    CORE_MUTEX_UNLOCKED
  );

  *mutex = (pthread_mutex_t)_Objects_Build_id( OBJECTS_POSIX_MUTEXES, _Objects_Get_index( the_mutex->Object.id ) );

  _Thread_Unnest_dispatch();
  return 0;
}

/*PAGE
 *
 *  11.3.2 Initializing and Destroying a Mutex, P1003.1c/Draft 10, p. 87
 */

int pthread_mutex_destroy(
  pthread_mutex_t            mutex
)
{
  POSIX_Mutex_Control *the_mutex;

  the_mutex = _POSIX_Mutex_Get( mutex );

  if ( !the_mutex )
    return 0;

  /*
   * XXX: There is an error for the mutex being locked
   *  or being in use by a condition variable.
   */

  if ( _CORE_mutex_Is_locked( &the_mutex->Mutex ) ) {
    _Thread_Enable_dispatch();
    return EBUSY;
  }

  _CORE_mutex_Flush(
    &the_mutex->Mutex,
    EINVAL
  );

  _Thread_Enable_dispatch();

  _POSIX_Mutex_Free( the_mutex );
  return 0;
}

/*PAGE
 *
 *  11.3.3 Locking and Unlocking a Mutex, P1003.1c/Draft 10, p. 93
 *        
 *  NOTE: P1003.4b/D8 adds pthread_mutex_timedlock(), p. 29
 */

int pthread_mutex_lock(
  pthread_mutex_t            mutex
)
{
  return _POSIX_Mutex_Lock_support( mutex, RTEMS_NO_TIMEOUT, TRUE );
}

/*PAGE
 *
 *  11.3.3 Locking and Unlocking a Mutex, P1003.1c/Draft 10, p. 93
 *        
 *  NOTE: P1003.4b/D8 adds pthread_mutex_timedlock(), p. 29
 */

int pthread_mutex_trylock(
  pthread_mutex_t            mutex
)
{
  return _POSIX_Mutex_Lock_support( mutex, RTEMS_NO_TIMEOUT, FALSE );
}

/*PAGE
 *
 *  11.3.3 Locking and Unlocking a Mutex, P1003.1c/Draft 10, p. 93
 *        
 *  NOTE: P1003.4b/D8 adds pthread_mutex_timedlock(), p. 29
 */

int pthread_mutex_unlock(
  pthread_mutex_t            mutex
)
{
  unsigned32           status;
  POSIX_Mutex_Control *the_mutex;

  the_mutex = _POSIX_Mutex_Get( mutex );

  if ( !the_mutex )
    rtems_set_errno_and_return_minus_one( EINVAL );

  status = _CORE_mutex_Surrender(
    &the_mutex->Mutex
  );

  _Thread_Enable_dispatch();

  return status;
}
