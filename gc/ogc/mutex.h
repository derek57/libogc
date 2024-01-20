/*  rtems/posix/mutex.h
 *
 *  This include file contains all the private support information for
 *  POSIX mutex's.
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


#ifndef __MUTEX_H__
#define __MUTEX_H__

/*! \file mutex.h 
\brief Thread subsystem III

*/ 

#ifdef __cplusplus
	extern "C" {
#endif

#include <gctypes.h>
#include "lwp_config.h"

/*
 *  Constant to indicate condition variable does not currently have
 *  a mutex assigned to it.
 */

#define POSIX_CONDITION_VARIABLES_NO_MUTEX		0xffffffff


/*! \typedef unsigned32 pthread_mutex_t
\brief typedef for the mutex handle
*/
typedef unsigned32 pthread_mutex_t;


/*! \fn int pthread_mutex_init(pthread_mutex_t *mutex,boolean use_recursive)
\brief Initializes a mutex lock.
\param[out] mutex pointer to a pthread_mutex_t handle.
\param[in] use_recursive whether to allow the thread, whithin the same context, to enter multiple times the lock or not.

\return 0 on success, <0 on error
*/
int pthread_mutex_init(
  pthread_mutex_t *mutex,
  bool use_recursive
);


/*! \fn int pthread_mutex_destroy(pthread_mutex_t mutex)
\brief Close mutex lock, release all threads and handles locked on this mutex.
\param[in] mutex handle to the pthread_mutex_t structure.

\return 0 on success, <0 on error
*/
int pthread_mutex_destroy(
  pthread_mutex_t mutex
);


/*! \fn int pthread_mutex_lock(pthread_mutex_t mutex)
\brief Enter the mutex lock.
\param[in] mutex handle to the mutext_t structure.

\return 0 on success, <0 on error
*/
int pthread_mutex_lock(
  pthread_mutex_t mutex
);


/*! \fn int pthread_mutex_trylock(pthread_mutex_t mutex)
\brief Try to enter the mutex lock.
\param[in] mutex handle to the pthread_mutex_t structure.

\return 0: on first aquire, 1: would lock
*/
int pthread_mutex_trylock(
  pthread_mutex_t mutex
);


/*! \fn int pthread_mutex_unlock(pthread_mutex_t mutex)
\brief Release the mutex lock and let other threads process further on this mutex.
\param[in] mutex handle to the pthread_mutex_t structure.

\return 0 on success, <0 on error
*/
int pthread_mutex_unlock(
  pthread_mutex_t mutex
);

#ifdef __cplusplus
	}
#endif

#endif
