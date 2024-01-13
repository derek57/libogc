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

#define LWP_OBJTYPE_COND				5

#define LWP_CHECK_COND(hndl)		\
{									\
	if(((hndl)==LWP_COND_NULL) || (LWP_OBJTYPE(hndl)!=LWP_OBJTYPE_COND))	\
		return NULL;				\
}

typedef struct {
	Objects_Control Object;
	pthread_mutex_t Mutex;
	Thread_queue_Control Wait_queue;
} POSIX_Condition_variables_Control;

Objects_Information _lwp_cond_objects;

extern int clock_gettime(struct timespec *tp);
extern void timespec_subtract(const struct timespec *tp_start,const struct timespec *tp_end,struct timespec *result);

void __lwp_cond_init()
{
	_Objects_Initialize_information(&_lwp_cond_objects,CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES,sizeof(POSIX_Condition_variables_Control));
}

static __inline__ POSIX_Condition_variables_Control* __lwp_cond_open(pthread_cond_t cond)
{
	LWP_CHECK_COND(cond);
	return (POSIX_Condition_variables_Control*)_Objects_Get(&_lwp_cond_objects,LWP_OBJMASKID(cond));
}

static __inline__ void __lwp_cond_free(POSIX_Condition_variables_Control *cond)
{
	_Objects_Close(&_lwp_cond_objects,&cond->Object);
	_Objects_Free(&_lwp_cond_objects,&cond->Object);
}

static POSIX_Condition_variables_Control* __lwp_cond_allocate()
{
	POSIX_Condition_variables_Control *cond;

	_Thread_Disable_dispatch();
	cond = (POSIX_Condition_variables_Control*)_Objects_Allocate(&_lwp_cond_objects);
	if(cond) {
		_Objects_Open(&_lwp_cond_objects,&cond->Object);
		return cond;
	}
	_Thread_Enable_dispatch();
	return NULL;
}

static s32 __lwp_cond_waitsupp(pthread_cond_t cond,pthread_mutex_t mutex,u64 timeout,u8 timedout)
{
	u32 status,mstatus,level;
	POSIX_Condition_variables_Control *thecond;

	thecond = __lwp_cond_open(cond);
	if(!thecond) return -1;
		
	if(thecond->Mutex!=LWP_MUTEX_NULL && thecond->Mutex!=mutex) {
		_Thread_Enable_dispatch();
		return EINVAL;
	}


	LWP_MutexUnlock(mutex);
	if(!timedout) {
		thecond->Mutex = mutex;
		_CPU_ISR_Disable(level);
		_Thread_queue_Enter_critical_section(&thecond->Wait_queue);
		_Thread_Executing->Wait.return_code = 0;
		_Thread_Executing->Wait.queue = &thecond->Wait_queue;
		_Thread_Executing->Wait.id = cond;
		_CPU_ISR_Restore(level);
		_Thread_queue_Enqueue(&thecond->Wait_queue,timeout);
		_Thread_Enable_dispatch();
		
		status = _Thread_Executing->Wait.return_code;
		if(status && status!=ETIMEDOUT)
			return status;
	} else {
		_Thread_Enable_dispatch();
		status = ETIMEDOUT;
	}

	mstatus = LWP_MutexLock(mutex);
	if(mstatus)
		return EINVAL;

	return status;
}

static s32 __lwp_cond_signalsupp(pthread_cond_t cond,u8 isbroadcast)
{
	Thread_Control *thethread;
	POSIX_Condition_variables_Control *thecond;
	
	thecond = __lwp_cond_open(cond);
	if(!thecond) return -1;

	do {
		thethread = _Thread_queue_Dequeue(&thecond->Wait_queue);
		if(!thethread) thecond->Mutex = LWP_MUTEX_NULL;
	} while(isbroadcast && thethread);
	_Thread_Enable_dispatch();
	return 0;
}

s32 LWP_CondInit(pthread_cond_t *cond)
{
	POSIX_Condition_variables_Control *ret;
	
	if(!cond) return -1;
	
	ret = __lwp_cond_allocate();
	if(!ret) return ENOMEM;

	ret->Mutex = LWP_MUTEX_NULL;
	_Thread_queue_Initialize(&ret->Wait_queue,THREAD_QUEUE_DISCIPLINE_FIFO,STATES_WAITING_FOR_CONDITION_VARIABLE,ETIMEDOUT);

	*cond = (pthread_cond_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_COND)|LWP_OBJMASKID(ret->Object.id));
	_Thread_Enable_dispatch();

	return 0;
}

s32 LWP_CondWait(pthread_cond_t cond,pthread_mutex_t mutex)
{
	return __lwp_cond_waitsupp(cond,mutex,RTEMS_NO_TIMEOUT,FALSE);
}

s32 LWP_CondSignal(pthread_cond_t cond)
{
	return __lwp_cond_signalsupp(cond,FALSE);
}

s32 LWP_CondBroadcast(pthread_cond_t cond)
{
	return __lwp_cond_signalsupp(cond,TRUE);
}

s32 LWP_CondTimedWait(pthread_cond_t cond,pthread_mutex_t mutex,const struct timespec *abstime)
{
	u64 timeout = RTEMS_NO_TIMEOUT;
	bool timedout = FALSE;

	if(abstime) timeout = _POSIX_Timespec_to_interval(abstime);
	return __lwp_cond_waitsupp(cond,mutex,timeout,timedout);
}

s32 LWP_CondDestroy(pthread_cond_t cond)
{
	POSIX_Condition_variables_Control *ptr;

	ptr = __lwp_cond_open(cond);
	if(!ptr) return -1;

	if(_Thread_queue_First(&ptr->Wait_queue)) {
		_Thread_Enable_dispatch();
		return EBUSY;
	}
	_Thread_Enable_dispatch();

	__lwp_cond_free(ptr);
	return 0;
}
