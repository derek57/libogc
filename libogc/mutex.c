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

#define LWP_CHECK_MUTEX(hndl)		\
{									\
	if(((hndl)==LWP_MUTEX_NULL) || (_Objects_Get_node(hndl)!=OBJECTS_POSIX_MUTEXES))	\
		return NULL;				\
}

typedef struct
{
	Objects_Control Object;
	CORE_mutex_Control Mutex;
} POSIX_Mutex_Control;

Objects_Information _lwp_mutex_objects;

static s32 __lwp_mutex_locksupp(pthread_mutex_t lock,u32 timeout,u8 block)
{
	u32 level;
	POSIX_Mutex_Control *p;

	if(lock==LWP_MUTEX_NULL || _Objects_Get_node(lock)!=OBJECTS_POSIX_MUTEXES) return -1;
	
	p = (POSIX_Mutex_Control*)_Objects_Get_isr_disable(&_lwp_mutex_objects,_Objects_Get_index(lock),&level);
	if(!p) return -1;

	_CORE_mutex_Seize(&p->Mutex,p->Object.id,block,timeout,level);
	return _Thread_Executing->Wait.return_code;
}

void __lwp_mutex_init()
{
	_Objects_Initialize_information(&_lwp_mutex_objects,CONFIGURE_MAXIMUM_POSIX_MUTEXES,sizeof(POSIX_Mutex_Control));
}


RTEMS_INLINE_ROUTINE POSIX_Mutex_Control* __lwp_mutex_open(pthread_mutex_t lock)
{
	LWP_CHECK_MUTEX(lock);
	return (POSIX_Mutex_Control*)_Objects_Get(&_lwp_mutex_objects,_Objects_Get_index(lock));
}

RTEMS_INLINE_ROUTINE void __lwp_mutex_free(POSIX_Mutex_Control *lock)
{
	_Objects_Close(&_lwp_mutex_objects,&lock->Object);
	_Objects_Free(&_lwp_mutex_objects,&lock->Object);
}

static POSIX_Mutex_Control* __lwp_mutex_allocate()
{
	POSIX_Mutex_Control *lock;

	_Thread_Disable_dispatch();
	lock = (POSIX_Mutex_Control*)_Objects_Allocate(&_lwp_mutex_objects);
	if(lock) {
		_Objects_Open(&_lwp_mutex_objects,&lock->Object);
		return lock;
	}
	_Thread_Unnest_dispatch();
	return NULL;
}

s32 LWP_MutexInit(pthread_mutex_t *mutex,bool use_recursive)
{
	CORE_mutex_Attributes attr;
	POSIX_Mutex_Control *ret;
	
	if(!mutex) return -1;

	ret = __lwp_mutex_allocate();
	if(!ret) return -1;

	attr.discipline = CORE_MUTEX_DISCIPLINES_FIFO;
	attr.lock_nesting_behavior = use_recursive?CORE_MUTEX_NESTING_ACQUIRES:CORE_MUTEX_NESTING_IS_ERROR;
	attr.only_owner_release = TRUE;
	attr.priority_ceiling = 1; //__lwp_priotocore(PRIORITY_MAXIMUM-1);
	_CORE_mutex_Initialize(&ret->Mutex,&attr,CORE_MUTEX_UNLOCKED);

	*mutex = (pthread_mutex_t)_Objects_Build_id(OBJECTS_POSIX_MUTEXES, _Objects_Get_index(ret->Object.id));
	_Thread_Unnest_dispatch();
	return 0;
}

s32 LWP_MutexDestroy(pthread_mutex_t mutex)
{
	POSIX_Mutex_Control *p;

	p = __lwp_mutex_open(mutex);
	if(!p) return 0;

	if(_CORE_mutex_Is_locked(&p->Mutex)) {
		_Thread_Enable_dispatch();
		return EBUSY;
	}
	_CORE_mutex_Flush(&p->Mutex,EINVAL);
	_Thread_Enable_dispatch();

	__lwp_mutex_free(p);
	return 0;
}

s32 LWP_MutexLock(pthread_mutex_t mutex)
{
	return __lwp_mutex_locksupp(mutex,RTEMS_NO_TIMEOUT,TRUE);
}

s32 LWP_MutexTryLock(pthread_mutex_t mutex)
{
	return __lwp_mutex_locksupp(mutex,RTEMS_NO_TIMEOUT,FALSE);
}

s32 LWP_MutexUnlock(pthread_mutex_t mutex)
{
	u32 ret;
	POSIX_Mutex_Control *lock;

	lock = __lwp_mutex_open(mutex);
	if(!lock) return -1;

	ret = _CORE_mutex_Surrender(&lock->Mutex);
	_Thread_Enable_dispatch();

	return ret;
}
