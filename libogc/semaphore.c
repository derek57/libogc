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
#include "lwp_sema.h"
#include "lwp_objmgr.h"
#include "lwp_config.h"
#include "semaphore.h"

#define LWP_CHECK_SEM(hndl)		\
{									\
	if(((hndl)==LWP_SEM_NULL) || (_Objects_Get_node(hndl)!=OBJECTS_POSIX_SEMAPHORES))	\
		return NULL;				\
}


typedef struct
{
	Objects_Control Object;
	CORE_semaphore_Control Semaphore;
} POSIX_Semaphore_Control;

Objects_Information _lwp_sema_objects;

void __lwp_sema_init()
{
	_Objects_Initialize_information(&_lwp_sema_objects,CONFIGURE_MAXIMUM_POSIX_SEMAPHORES,sizeof(POSIX_Semaphore_Control));
}

RTEMS_INLINE_ROUTINE POSIX_Semaphore_Control* __lwp_sema_open(sem_t sem)
{
	LWP_CHECK_SEM(sem);
	return (POSIX_Semaphore_Control*)_Objects_Get(&_lwp_sema_objects,_Objects_Get_index(sem));
}

RTEMS_INLINE_ROUTINE void __lwp_sema_free(POSIX_Semaphore_Control *sema)
{
	_Objects_Close(&_lwp_sema_objects,&sema->Object);
	_Objects_Free(&_lwp_sema_objects,&sema->Object);
}

static POSIX_Semaphore_Control* __lwp_sema_allocate()
{
	POSIX_Semaphore_Control *sema;

	_Thread_Disable_dispatch();
	sema = (POSIX_Semaphore_Control*)_Objects_Allocate(&_lwp_sema_objects);
	if(sema) {
		_Objects_Open(&_lwp_sema_objects,&sema->Object);
		return sema;
	}
	_Thread_Enable_dispatch();
	return NULL;
}

s32 LWP_SemInit(sem_t *sem,u32 start,u32 max)
{
	CORE_semaphore_Attributes attr;
	POSIX_Semaphore_Control *ret;

	if(!sem) return -1;
	
	ret = __lwp_sema_allocate();
	if(!ret) return -1;

	attr.maximum_count = max;
	attr.discipline = CORE_SEMAPHORE_DISCIPLINES_FIFO;
	_CORE_semaphore_Initialize(&ret->Semaphore,&attr,start);

	*sem = (sem_t)_Objects_Build_id(OBJECTS_POSIX_SEMAPHORES, _Objects_Get_index(ret->Object.id));
	_Thread_Enable_dispatch();
	return 0;
}

s32 LWP_SemWait(sem_t sem)
{
	POSIX_Semaphore_Control *lwp_sem;

	lwp_sem = __lwp_sema_open(sem);
	if(!lwp_sem) return -1;

	_CORE_semaphore_Seize(&lwp_sem->Semaphore,lwp_sem->Object.id,TRUE,RTEMS_NO_TIMEOUT);
	_Thread_Enable_dispatch();

	switch(_Thread_Executing->Wait.return_code) {
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

s32 LWP_SemPost(sem_t sem)
{
	POSIX_Semaphore_Control *lwp_sem;

	lwp_sem = __lwp_sema_open(sem);
	if(!lwp_sem) return -1;

	_CORE_semaphore_Surrender(&lwp_sem->Semaphore,lwp_sem->Object.id);
	_Thread_Enable_dispatch();

	return 0;
}

s32 LWP_SemDestroy(sem_t sem)
{
	POSIX_Semaphore_Control *lwp_sem;

	lwp_sem = __lwp_sema_open(sem);
	if(!lwp_sem) return -1;

	_CORE_semaphore_Flush(&lwp_sem->Semaphore,-1);
	_Thread_Enable_dispatch();

	__lwp_sema_free(lwp_sem);
	return 0;
}
