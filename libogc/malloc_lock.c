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
static lwp_mutex mem_lock;

void __memlock_init()
{
	_Thread_Disable_dispatch();
	if(!initialized) {
		lwp_mutex_attr attr;

		initialized = 1;
		
		attr.mode = LWP_MUTEX_FIFO;
		attr.nest_behavior = LWP_MUTEX_NEST_ACQUIRE;
		attr.onlyownerrelease = TRUE;
		attr.prioceil = 1;
		_CORE_mutex_Initialize(&mem_lock,&attr,LWP_MUTEX_UNLOCKED);
	}
	_Thread_Unnest_dispatch();
}

#ifndef REENTRANT_SYSCALLS_PROVIDED
void _DEFUN(__libogc_malloc_lock,(r),
			struct _reent *r)
{
	unsigned int level;
	
	if(!initialized) return;

	_CPU_ISR_Disable(level);
	_CORE_mutex_Seize(&mem_lock,MEMLOCK_MUTEX_ID,TRUE,LWP_THREADQ_NOTIMEOUT,level);
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
	unsigned int level;
	
	if(!initialized) return;

	_CPU_ISR_Disable(level);
	_CORE_mutex_Seize(&mem_lock,MEMLOCK_MUTEX_ID,TRUE,LWP_THREADQ_NOTIMEOUT,level);
	ptr->_errno = _thr_executing->wait.ret_code;
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
