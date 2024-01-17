#include <_ansi.h>
#include <_syslist.h>
#ifndef REENTRANT_SYSCALLS_PROVIDED
#include <reent.h>
#endif
#include <errno.h>
#undef errno
extern int errno;

#include "asm.h"
#include "processor.h"
#include "lwp_mutex.h"

#define MEMLOCK_MUTEX_ID			0x00030040

static int initialized = 0;
static CORE_mutex_Control mem_lock;

void __memlock_init()
{
	_Thread_Disable_dispatch();
	if(!initialized) {
		CORE_mutex_Attributes attr;

		initialized = 1;
		
		attr.discipline = CORE_MUTEX_DISCIPLINES_FIFO;
		attr.lock_nesting_behavior = CORE_MUTEX_NESTING_ACQUIRES;
		attr.only_owner_release = TRUE;
		attr.priority_ceiling = 1;
		_CORE_mutex_Initialize(&mem_lock,&attr,CORE_MUTEX_UNLOCKED);
	}
	_Thread_Unnest_dispatch();
}

#ifndef REENTRANT_SYSCALLS_PROVIDED
void _DEFUN(__libogc_malloc_lock,(r),
			struct _reent *r)
{
	ISR_Level level;
	
	if(!initialized) return;

	_ISR_Disable(level);
	_CORE_mutex_Seize(&mem_lock,MEMLOCK_MUTEX_ID,TRUE,RTEMS_NO_TIMEOUT,level);
}

void _DEFUN(__libogc_malloc_unlock,(r),
			struct _reent *r)
{
	if(!initialized) return;

	_Thread_Disable_dispatch();
	_CORE_mutex_Surrender(&mem_lock);
	_Thread_Enable_dispatch();
}

#else
void _DEFUN(__libogc_malloc_lock,(ptr),
			struct _reent *ptr)
{
	ISR_Level level;
	
	if(!initialized) return;

	_ISR_Disable(level);
	_CORE_mutex_Seize(&mem_lock,MEMLOCK_MUTEX_ID,TRUE,RTEMS_NO_TIMEOUT,level);
	ptr->_errno = _Thread_Executing->wait.ret_code;
}

void _DEFUN(__libogc_malloc_unlock,(ptr),
			struct _reent *ptr)
{
	if(!initialized) return;

	_Thread_Disable_dispatch();
	ptr->_errno = _CORE_mutex_Surrender(&mem_lock);
	_Thread_Enable_dispatch();
}

#endif
