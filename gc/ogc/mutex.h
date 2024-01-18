/*-------------------------------------------------------------

mutex.h -- Thread subsystem III

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


/*! \fn signed32 LWP_MutexInit(pthread_mutex_t *mutex,boolean use_recursive)
\brief Initializes a mutex lock.
\param[out] mutex pointer to a pthread_mutex_t handle.
\param[in] use_recursive whether to allow the thread, whithin the same context, to enter multiple times the lock or not.

\return 0 on success, <0 on error
*/
signed32 LWP_MutexInit(pthread_mutex_t *mutex,bool use_recursive);


/*! \fn signed32 LWP_MutexDestroy(pthread_mutex_t mutex)
\brief Close mutex lock, release all threads and handles locked on this mutex.
\param[in] mutex handle to the pthread_mutex_t structure.

\return 0 on success, <0 on error
*/
signed32 LWP_MutexDestroy(pthread_mutex_t mutex);


/*! \fn signed32 pthread_mutex_lock(pthread_mutex_t mutex)
\brief Enter the mutex lock.
\param[in] mutex handle to the mutext_t structure.

\return 0 on success, <0 on error
*/
signed32 pthread_mutex_lock(pthread_mutex_t mutex);


/*! \fn signed32 LWP_MutexTryLock(pthread_mutex_t mutex)
\brief Try to enter the mutex lock.
\param[in] mutex handle to the pthread_mutex_t structure.

\return 0: on first aquire, 1: would lock
*/
signed32 LWP_MutexTryLock(pthread_mutex_t mutex);


/*! \fn signed32 pthread_mutex_unlock(pthread_mutex_t mutex)
\brief Release the mutex lock and let other threads process further on this mutex.
\param[in] mutex handle to the pthread_mutex_t structure.

\return 0 on success, <0 on error
*/
signed32 pthread_mutex_unlock(pthread_mutex_t mutex);

#ifdef __cplusplus
	}
#endif

#endif
