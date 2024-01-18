/*  rtems/posix/cond.h
 *
 *  This include file contains all the private support information for
 *  POSIX condition variables.
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

#ifndef __COND_H__
#define __COND_H__

/*! \file cond.h 
\brief Thread subsystem V

*/ 

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <time.h>

#define LWP_COND_NULL		0xffffffff


/*! \typedef unsigned32 pthread_cond_t
\brief typedef for the condition variable handle
*/
typedef unsigned32 pthread_cond_t;


/*! \fn int pthread_cond_init(pthread_cond_t *cond)
\brief Initialize condition variable
\param[out] cond pointer to the pthread_cond_t handle

\return 0 on success, <0 on error
*/
int pthread_cond_init(pthread_cond_t *cond);


/*! \fn int pthread_cond_wait(pthread_cond_t cond,pthread_mutex_t mutex)
\brief Wait on condition variable. 
\param[in] cond handle to the pthread_cond_t structure
\param[in] mutex handle to the pthread_mutex_t structure

\return 0 on success, <0 on error
*/
int pthread_cond_wait(pthread_cond_t cond,pthread_mutex_t mutex);


/*! \fn int pthread_cond_signal(pthread_cond_t cond)
\brief Signal a specific thread waiting on this condition variable to wake up.
\param[in] cond handle to the pthread_cond_t structure

\return 0 on success, <0 on error
*/
int pthread_cond_signal(pthread_cond_t cond);


/*! \fn int pthread_cond_broadcast(pthread_cond_t cond)
\brief Broadcast all threads waiting on this condition variable to wake up.
\param[in] cond handle to the pthread_cond_t structure

\return 0 on success, <0 on error
*/
int pthread_cond_broadcast(pthread_cond_t cond);


/*! \fn int pthread_cond_timedwait(pthread_cond_t cond,pthread_mutex_t mutex,const struct timespec *abstime)
\brief Timed wait on a conditionvariable.
\param[in] cond handle to the pthread_cond_t structure
\param[in] mutex handle to the pthread_mutex_t structure
\param[in] abstime pointer to a timespec structure holding the abs time for the timeout.

\return 0 on success, <0 on error
*/
int pthread_cond_timedwait(pthread_cond_t cond,pthread_mutex_t mutex,const struct timespec *abstime);


/*! \fn int pthread_cond_destroy(pthread_cond_t cond)
\brief Destroy condition variable, release all threads and handles blocked on that condition variable.
\param[in] cond handle to the pthread_cond_t structure

\return 0 on success, <0 on error
*/
int pthread_cond_destroy(pthread_cond_t cond);

#ifdef __cplusplus
	}
#endif

#endif
