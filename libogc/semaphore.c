/*-------------------------------------------------------------

semaphore.c -- Thread subsystem IV

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
#include <asm.h>
#include "coresem.h"
#include "object.h"
#include "lwp_config.h"
#include "semaphore.h"

#define LWP_CHECK_SEM( hndl )   \
{                               \
  if ( ( ( hndl ) == SEM_FAILED ) || ( _Objects_Get_node( hndl ) != OBJECTS_POSIX_SEMAPHORES ) )  \
    return NULL;                \
}

/*
 *  Data Structure used to manage a POSIX semaphore
 */

typedef struct
{
   Objects_Control Object;
   CORE_semaphore_Control Semaphore;
}  POSIX_Semaphore_Control;

/*
 *  The following defines the information control block used to manage
 *  this class of objects.
 */

Objects_Information _POSIX_Semaphore_Information;

/*PAGE
 *
 *  _POSIX_Semaphore_Manager_initialization
 *
 *  This routine initializes all semaphore manager related data structures.
 *
 *  Input parameters:
 *    maximum_semaphores - maximum configured semaphores
 *
 *  Output parameters:  NONE
 */

void _POSIX_Semaphore_Manager_initialization( void )
{
  _Objects_Initialize_information(
    &_POSIX_Semaphore_Information,
    CONFIGURE_MAXIMUM_POSIX_SEMAPHORES,
    sizeof( POSIX_Semaphore_Control )
  );
}

/*PAGE
 *
 *  _POSIX_Semaphore_Get
 */

RTEMS_INLINE_ROUTINE POSIX_Semaphore_Control *_POSIX_Semaphore_Get (
  sem_t         sem
)
{
  LWP_CHECK_SEM( sem );

  return (POSIX_Semaphore_Control *)_Objects_Get(
    &_POSIX_Semaphore_Information,
    _Objects_Get_index( sem )
  );
}

/*PAGE
 *
 *  _POSIX_Semaphore_Free
 */

RTEMS_INLINE_ROUTINE void _POSIX_Semaphore_Free (
  POSIX_Semaphore_Control *the_semaphore
)
{
  _Objects_Close( &_POSIX_Semaphore_Information, &the_semaphore->Object );
  _Objects_Free( &_POSIX_Semaphore_Information, &the_semaphore->Object );
}

/*PAGE
 *
 *  _POSIX_Semaphore_Allocate
 */

STATIC POSIX_Semaphore_Control *_POSIX_Semaphore_Allocate( void )
{
  POSIX_Semaphore_Control *the_semaphore;

  _Thread_Disable_dispatch();

  the_semaphore = (POSIX_Semaphore_Control *)_Objects_Allocate( &_POSIX_Semaphore_Information );

  if ( the_semaphore ) {
    _Objects_Open( &_POSIX_Semaphore_Information, &the_semaphore->Object );
    return the_semaphore;
  }

  _Thread_Enable_dispatch();

  return NULL;
}

/*PAGE
 *
 *  11.2.1 Initialize an Unnamed Semaphore, P1003.1b-1993, p.219
 */

int sem_init(
  sem_t         *sem,
  unsigned32     value,
  unsigned32     max
)
{
  CORE_semaphore_Attributes  the_sem_attr;
  POSIX_Semaphore_Control   *the_semaphore;

  if ( !sem )
    rtems_set_errno_and_return_minus_one( EINVAL );

  the_semaphore = _POSIX_Semaphore_Allocate();

  if ( !the_semaphore )
    rtems_set_errno_and_return_minus_one( ENOSPC );

  /*
   *  This effectively disables limit checking.
   */

  the_sem_attr.maximum_count = max;

  /*
   *  POSIX does not appear to specify what the discipline for 
   *  blocking tasks on this semaphore should be.  It could somehow 
   *  be derived from the current scheduling policy.  One
   *  thing is certain, no matter what we decide, it won't be 
   *  the same as  all other POSIX implementations. :)
   */

  the_sem_attr.discipline = CORE_SEMAPHORE_DISCIPLINES_FIFO;

  _CORE_semaphore_Initialize(
    &the_semaphore->Semaphore,
    &the_sem_attr,
    value
  );

  *sem = (sem_t)_Objects_Build_id( OBJECTS_POSIX_SEMAPHORES, _Objects_Get_index( the_semaphore->Object.id ) );

  _Thread_Enable_dispatch();

  return 0;
}

/*PAGE
 *
 *  11.2.6 Lock a Semaphore, P1003.1b-1993, p.226
 *
 *  NOTE: P1003.4b/D8 adds sem_timedwait(), p. 27
 */

int sem_wait(
  sem_t               sem
)
{
  POSIX_Semaphore_Control *the_semaphore;

  the_semaphore = _POSIX_Semaphore_Get( sem );

  if ( !the_semaphore )
    rtems_set_errno_and_return_minus_one( EINVAL );

  _CORE_semaphore_Seize(
    &the_semaphore->Semaphore,
    the_semaphore->Object.id,
    TRUE,
    RTEMS_NO_TIMEOUT
  );

  _Thread_Enable_dispatch();

  switch ( _Thread_Executing->Wait.return_code ) {
    case CORE_SEMAPHORE_STATUS_SUCCESSFUL:
      break;
    case CORE_SEMAPHORE_STATUS_UNSATISFIED_NOWAIT:
      return EAGAIN;
    case CORE_SEMAPHORE_WAS_DELETED:
      return EAGAIN;
    case CORE_SEMAPHORE_TIMEOUT:
      return ETIMEDOUT;
	}
  return 0;
}

/*PAGE
 *
 *  11.2.7 Unlock a Semaphore, P1003.1b-1993, p.227
 */

int sem_post(
  sem_t sem
)
{
  POSIX_Semaphore_Control *the_semaphore;

  the_semaphore = _POSIX_Semaphore_Get( sem );

  if ( !the_semaphore )
    rtems_set_errno_and_return_minus_one( EINVAL );

  _CORE_semaphore_Surrender(
    &the_semaphore->Semaphore,
    the_semaphore->Object.id
  );

  _Thread_Enable_dispatch();
  return 0;
}

/*PAGE
 *
 *  11.2.2 Destroy an Unnamed Semaphore, P1003.1b-1993, p.220
 */

int sem_destroy(
  sem_t sem
)
{
  POSIX_Semaphore_Control *the_semaphore;

  the_semaphore = _POSIX_Semaphore_Get( sem );

  if ( !the_semaphore )
    rtems_set_errno_and_return_minus_one( EINVAL );

  _CORE_semaphore_Flush(
    &the_semaphore->Semaphore,
    -1  /* XXX should also seterrno -> EINVAL */ 
  );

  _Thread_Enable_dispatch();

  _POSIX_Semaphore_Free( the_semaphore );
  return 0;
}
