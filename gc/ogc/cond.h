/*-------------------------------------------------------------

cond.h -- Thread subsystem V

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


#ifndef __COND_H__
#define __COND_H__

/*! \file cond.h 
\brief Thread subsystem V

*/ 

#include <gctypes.h>
#include <time.h>

#define LWP_COND_NULL		0xffffffff

#ifdef __cplusplus
extern "C" {
#endif


/*! \typedef u32 pthread_cond_t
\brief typedef for the condition variable handle
*/
typedef u32 pthread_cond_t;


/*! \fn s32 LWP_CondInit(pthread_cond_t *cond)
\brief Initialize condition variable
\param[out] cond pointer to the pthread_cond_t handle

\return 0 on success, <0 on error
*/
s32 LWP_CondInit(pthread_cond_t *cond);


/*! \fn s32 LWP_CondWait(pthread_cond_t cond,pthread_mutex_t mutex)
\brief Wait on condition variable. 
\param[in] cond handle to the pthread_cond_t structure
\param[in] mutex handle to the pthread_mutex_t structure

\return 0 on success, <0 on error
*/
s32 LWP_CondWait(pthread_cond_t cond,pthread_mutex_t mutex);


/*! \fn s32 LWP_CondSignal(pthread_cond_t cond)
\brief Signal a specific thread waiting on this condition variable to wake up.
\param[in] cond handle to the pthread_cond_t structure

\return 0 on success, <0 on error
*/
s32 LWP_CondSignal(pthread_cond_t cond);


/*! \fn s32 LWP_CondBroadcast(pthread_cond_t cond)
\brief Broadcast all threads waiting on this condition variable to wake up.
\param[in] cond handle to the pthread_cond_t structure

\return 0 on success, <0 on error
*/
s32 LWP_CondBroadcast(pthread_cond_t cond);


/*! \fn s32 LWP_CondTimedWait(pthread_cond_t cond,pthread_mutex_t mutex,const struct timespec *abstime)
\brief Timed wait on a conditionvariable.
\param[in] cond handle to the pthread_cond_t structure
\param[in] mutex handle to the pthread_mutex_t structure
\param[in] abstime pointer to a timespec structure holding the abs time for the timeout.

\return 0 on success, <0 on error
*/
s32 LWP_CondTimedWait(pthread_cond_t cond,pthread_mutex_t mutex,const struct timespec *abstime);


/*! \fn s32 LWP_CondDestroy(pthread_cond_t cond)
\brief Destroy condition variable, release all threads and handles blocked on that condition variable.
\param[in] cond handle to the pthread_cond_t structure

\return 0 on success, <0 on error
*/
s32 LWP_CondDestroy(pthread_cond_t cond);

#ifdef __cplusplus
	}
#endif

#endif
