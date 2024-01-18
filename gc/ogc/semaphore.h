/*  rtems/posix/semaphore.h
 *
 *  This include file contains all the private support information for
 *  POSIX Semaphores.
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

#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

/*! \file semaphore.h 
\brief Thread subsystem IV

*/ 

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>

#define SEM_FAILED              0xffffffff

/*! \typedef unsigned32 sem_t
\brief typedef for the semaphore handle
*/
typedef unsigned32 sem_t;

/*! \fn int sem_init(sem_t *sem,unsigned32 start,unsigned32 max)
\brief Initializes a semaphore.
\param[out] sem pointer to a sem_t handle.
\param[in] start start count of the semaphore
\param[in] max maximum count of the semaphore

\return 0 on success, <0 on error
*/
int sem_init(sem_t *sem,unsigned32 start,unsigned32 max);


/*! \fn int sem_destroy(sem_t sem)
\brief Close and destroy a semaphore, release all threads and handles locked on this semaphore.
\param[in] sem handle to the sem_t structure.

\return 0 on success, <0 on error
*/
int sem_destroy(sem_t sem);


/*! \fn int sem_wait(sem_t sem)
\brief Count down semaphore counter and enter lock if counter <=0
\param[in] sem handle to the sem_t structure.

\return 0 on success, <0 on error
*/
int sem_wait(sem_t sem);


/*! \fn int sem_post(sem_t sem)
\brief Count up semaphore counter and release lock if counter >0
\param[in] sem handle to the sem_t structure.

\return 0 on success, <0 on error
*/
int sem_post(sem_t sem);

#ifdef __cplusplus
	}
#endif

#endif
