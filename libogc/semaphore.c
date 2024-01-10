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

#define LWP_OBJTYPE_SEM				4

#define LWP_CHECK_SEM(hndl)		\
{									\
	if(((hndl)==LWP_SEM_NULL) || (LWP_OBJTYPE(hndl)!=LWP_OBJTYPE_SEM))	\
		return NULL;				\
}


typedef struct _sema_st
{
	Objects_Control object;
	CORE_semaphore_Control sema;
} sema_st;

Objects_Information _lwp_sema_objects;

void __lwp_sema_init()
{
	_Objects_Initialize_information(&_lwp_sema_objects,LWP_MAX_SEMAS,sizeof(sema_st));
}

static __inline__ sema_st* __lwp_sema_open(sem_t sem)
{
	LWP_CHECK_SEM(sem);
	return (sema_st*)_Objects_Get(&_lwp_sema_objects,LWP_OBJMASKID(sem));
}

static __inline__ void __lwp_sema_free(sema_st *sema)
{
	_Objects_Close(&_lwp_sema_objects,&sema->object);
	_Objects_Free(&_lwp_sema_objects,&sema->object);
}

static sema_st* __lwp_sema_allocate()
{
	sema_st *sema;

	_Thread_Disable_dispatch();
	sema = (sema_st*)_Objects_Allocate(&_lwp_sema_objects);
	if(sema) {
		_Objects_Open(&_lwp_sema_objects,&sema->object);
		return sema;
	}
	_Thread_Enable_dispatch();
	return NULL;
}

s32 LWP_SemInit(sem_t *sem,u32 start,u32 max)
{
	CORE_semaphore_Attributes attr;
	sema_st *ret;

	if(!sem) return -1;
	
	ret = __lwp_sema_allocate();
	if(!ret) return -1;

	attr.max_cnt = max;
	attr.mode = LWP_SEMA_MODEFIFO;
	CORE_semaphore_Initialize(&ret->sema,&attr,start);

	*sem = (sem_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_SEM)|LWP_OBJMASKID(ret->object.id));
	_Thread_Enable_dispatch();
	return 0;
}

s32 LWP_SemWait(sem_t sem)
{
	sema_st *lwp_sem;

	lwp_sem = __lwp_sema_open(sem);
	if(!lwp_sem) return -1;

	_CORE_semaphore_Seize(&lwp_sem->sema,lwp_sem->object.id,TRUE,LWP_THREADQ_NOTIMEOUT);
	_Thread_Enable_dispatch();

	switch(_thr_executing->wait.ret_code) {
		case LWP_SEMA_SUCCESSFUL:
			break;
		case LWP_SEMA_UNSATISFIED_NOWAIT:
			return EAGAIN;
		case LWP_SEMA_DELETED:
			return EAGAIN;
		case LWP_SEMA_TIMEOUT:
			return ETIMEDOUT;
			
	}
	return 0;
}

s32 LWP_SemPost(sem_t sem)
{
	sema_st *lwp_sem;

	lwp_sem = __lwp_sema_open(sem);
	if(!lwp_sem) return -1;

	_CORE_semaphore_Surrender(&lwp_sem->sema,lwp_sem->object.id);
	_Thread_Enable_dispatch();

	return 0;
}

s32 LWP_SemDestroy(sem_t sem)
{
	sema_st *lwp_sem;

	lwp_sem = __lwp_sema_open(sem);
	if(!lwp_sem) return -1;

	_CORE_semaphore_Flush(&lwp_sem->sema,-1);
	_Thread_Enable_dispatch();

	__lwp_sema_free(lwp_sem);
	return 0;
}
