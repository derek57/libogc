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

typedef struct _cond_st {
	Objects_Control object;
	mutex_t lock;
	Thread_queue_Control wait_queue;
} cond_st;

Objects_Information _lwp_cond_objects;

extern int clock_gettime(struct timespec *tp);
extern void timespec_subtract(const struct timespec *tp_start,const struct timespec *tp_end,struct timespec *result);

void __lwp_cond_init()
{
	_Objects_Initialize_information(&_lwp_cond_objects,LWP_MAX_CONDVARS,sizeof(cond_st));
}

static __inline__ cond_st* __lwp_cond_open(cond_t cond)
{
	LWP_CHECK_COND(cond);
	return (cond_st*)_Objects_Get(&_lwp_cond_objects,LWP_OBJMASKID(cond));
}

static __inline__ void __lwp_cond_free(cond_st *cond)
{
	_Objects_Close(&_lwp_cond_objects,&cond->object);
	_Objects_Free(&_lwp_cond_objects,&cond->object);
}

static cond_st* __lwp_cond_allocate()
{
	cond_st *cond;

	_Thread_Disable_dispatch();
	cond = (cond_st*)_Objects_Allocate(&_lwp_cond_objects);
	if(cond) {
		_Objects_Open(&_lwp_cond_objects,&cond->object);
		return cond;
	}
	_Thread_Enable_dispatch();
	return NULL;
}

static s32 __lwp_cond_waitsupp(cond_t cond,mutex_t mutex,u64 timeout,u8 timedout)
{
	u32 status,mstatus,level;
	cond_st *thecond;

	thecond = __lwp_cond_open(cond);
	if(!thecond) return -1;
		
	if(thecond->lock!=LWP_MUTEX_NULL && thecond->lock!=mutex) {
		_Thread_Enable_dispatch();
		return EINVAL;
	}


	LWP_MutexUnlock(mutex);
	if(!timedout) {
		thecond->lock = mutex;
		_CPU_ISR_Disable(level);
		_Thread_queue_Enter_critical_section(&thecond->wait_queue);
		_Thread_Executing->Wait.return_code = 0;
		_Thread_Executing->Wait.queue = &thecond->wait_queue;
		_Thread_Executing->Wait.id = cond;
		_CPU_ISR_Restore(level);
		_Thread_queue_Enqueue(&thecond->wait_queue,timeout);
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

static s32 __lwp_cond_signalsupp(cond_t cond,u8 isbroadcast)
{
	Thread_Control *thethread;
	cond_st *thecond;
	
	thecond = __lwp_cond_open(cond);
	if(!thecond) return -1;

	do {
		thethread = _Thread_queue_Dequeue(&thecond->wait_queue);
		if(!thethread) thecond->lock = LWP_MUTEX_NULL;
	} while(isbroadcast && thethread);
	_Thread_Enable_dispatch();
	return 0;
}

s32 LWP_CondInit(cond_t *cond)
{
	cond_st *ret;
	
	if(!cond) return -1;
	
	ret = __lwp_cond_allocate();
	if(!ret) return ENOMEM;

	ret->lock = LWP_MUTEX_NULL;
	_Thread_queue_Initialize(&ret->wait_queue,LWP_THREADQ_MODEFIFO,LWP_STATES_WAITING_FOR_CONDVAR,ETIMEDOUT);

	*cond = (cond_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_COND)|LWP_OBJMASKID(ret->object.id));
	_Thread_Enable_dispatch();

	return 0;
}

s32 LWP_CondWait(cond_t cond,mutex_t mutex)
{
	return __lwp_cond_waitsupp(cond,mutex,LWP_THREADQ_NOTIMEOUT,FALSE);
}

s32 LWP_CondSignal(cond_t cond)
{
	return __lwp_cond_signalsupp(cond,FALSE);
}

s32 LWP_CondBroadcast(cond_t cond)
{
	return __lwp_cond_signalsupp(cond,TRUE);
}

s32 LWP_CondTimedWait(cond_t cond,mutex_t mutex,const struct timespec *abstime)
{
	u64 timeout = LWP_THREADQ_NOTIMEOUT;
	bool timedout = FALSE;

	if(abstime) timeout = _POSIX_Timespec_to_interval(abstime);
	return __lwp_cond_waitsupp(cond,mutex,timeout,timedout);
}

s32 LWP_CondDestroy(cond_t cond)
{
	cond_st *ptr;

	ptr = __lwp_cond_open(cond);
	if(!ptr) return -1;

	if(_Thread_queue_First(&ptr->wait_queue)) {
		_Thread_Enable_dispatch();
		return EBUSY;
	}
	_Thread_Enable_dispatch();

	__lwp_cond_free(ptr);
	return 0;
}
