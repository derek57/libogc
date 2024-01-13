/*-------------------------------------------------------------

mutex.c -- Thread subsystem III

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
#include "lwp_mutex.h"
#include "lwp_objmgr.h"
#include "lwp_config.h"
#include "mutex.h"

#define LWP_OBJTYPE_MUTEX			3

#define LWP_CHECK_MUTEX(hndl)		\
{									\
	if(((hndl)==LWP_MUTEX_NULL) || (LWP_OBJTYPE(hndl)!=LWP_OBJTYPE_MUTEX))	\
		return NULL;				\
}

typedef struct _mutex_st
{
	Objects_Control object;
	CORE_mutex_Control mutex;
} mutex_st;

Objects_Information _lwp_mutex_objects;

static s32 __lwp_mutex_locksupp(mutex_t lock,u32 timeout,u8 block)
{
	u32 level;
	mutex_st *p;

	if(lock==LWP_MUTEX_NULL || LWP_OBJTYPE(lock)!=LWP_OBJTYPE_MUTEX) return -1;
	
	p = (mutex_st*)_Objects_Get_isr_disable(&_lwp_mutex_objects,LWP_OBJMASKID(lock),&level);
	if(!p) return -1;

	_CORE_mutex_Seize(&p->mutex,p->object.id,block,timeout,level);
	return _Thread_Executing->Wait.return_code;
}

void __lwp_mutex_init()
{
	_Objects_Initialize_information(&_lwp_mutex_objects,LWP_MAX_MUTEXES,sizeof(mutex_st));
}


static __inline__ mutex_st* __lwp_mutex_open(mutex_t lock)
{
	LWP_CHECK_MUTEX(lock);
	return (mutex_st*)_Objects_Get(&_lwp_mutex_objects,LWP_OBJMASKID(lock));
}

static __inline__ void __lwp_mutex_free(mutex_st *lock)
{
	_Objects_Close(&_lwp_mutex_objects,&lock->object);
	_Objects_Free(&_lwp_mutex_objects,&lock->object);
}

static mutex_st* __lwp_mutex_allocate()
{
	mutex_st *lock;

	_Thread_Disable_dispatch();
	lock = (mutex_st*)_Objects_Allocate(&_lwp_mutex_objects);
	if(lock) {
		_Objects_Open(&_lwp_mutex_objects,&lock->object);
		return lock;
	}
	_Thread_Unnest_dispatch();
	return NULL;
}

s32 LWP_MutexInit(mutex_t *mutex,bool use_recursive)
{
	CORE_mutex_Attributes attr;
	mutex_st *ret;
	
	if(!mutex) return -1;

	ret = __lwp_mutex_allocate();
	if(!ret) return -1;

	attr.discipline = CORE_MUTEX_DISCIPLINES_FIFO;
	attr.lock_nesting_behavior = use_recursive?CORE_MUTEX_NESTING_ACQUIRES:CORE_MUTEX_NESTING_IS_ERROR;
	attr.only_owner_release = TRUE;
	attr.priority_ceiling = 1; //__lwp_priotocore(PRIORITY_MAXIMUM-1);
	_CORE_mutex_Initialize(&ret->mutex,&attr,CORE_MUTEX_UNLOCKED);

	*mutex = (mutex_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_MUTEX)|LWP_OBJMASKID(ret->object.id));
	_Thread_Unnest_dispatch();
	return 0;
}

s32 LWP_MutexDestroy(mutex_t mutex)
{
	mutex_st *p;

	p = __lwp_mutex_open(mutex);
	if(!p) return 0;

	if(_CORE_mutex_Is_locked(&p->mutex)) {
		_Thread_Enable_dispatch();
		return EBUSY;
	}
	_CORE_mutex_Flush(&p->mutex,EINVAL);
	_Thread_Enable_dispatch();

	__lwp_mutex_free(p);
	return 0;
}

s32 LWP_MutexLock(mutex_t mutex)
{
	return __lwp_mutex_locksupp(mutex,LWP_THREADQ_NOTIMEOUT,TRUE);
}

s32 LWP_MutexTryLock(mutex_t mutex)
{
	return __lwp_mutex_locksupp(mutex,LWP_THREADQ_NOTIMEOUT,FALSE);
}

s32 LWP_MutexUnlock(mutex_t mutex)
{
	u32 ret;
	mutex_st *lock;

	lock = __lwp_mutex_open(mutex);
	if(!lock) return -1;

	ret = _CORE_mutex_Surrender(&lock->mutex);
	_Thread_Enable_dispatch();

	return ret;
}
