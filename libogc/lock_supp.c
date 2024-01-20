#include <_ansi.h>
#include <_syslist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef REENTRANT_SYSCALLS_PROVIDED
#include <reent.h>
#endif
#include <errno.h>

#include "asm.h"
#include "processor.h"
#include "mutex.h"


int __libogc_lock_init(int *lock,int recursive)
{
	signed32 ret;
	pthread_mutex_t retlck = POSIX_CONDITION_VARIABLES_NO_MUTEX;

	if(!lock) return -1;
	
	*lock = 0;
	ret = pthread_mutex_init(&retlck,(recursive?TRUE:FALSE));
	if(ret==0) *lock = (int)retlck;

	return ret;
}

int __libogc_lock_close(int *lock)
{
	signed32 ret;
	pthread_mutex_t plock;
	
	if(!lock || *lock==0) return -1;
	
	plock = (pthread_mutex_t)*lock;
	ret = pthread_mutex_destroy(plock);
	if(ret==0) *lock = 0;

	return ret;
}

int __libogc_lock_acquire(int *lock)
{
	pthread_mutex_t plock;
	
	if(!lock || *lock==0) return -1;

	plock = (pthread_mutex_t)*lock;
	return pthread_mutex_lock(plock);
}


int __libogc_lock_release(int *lock)
{
	pthread_mutex_t plock;
	
	if(!lock || *lock==0) return -1;

	plock = (pthread_mutex_t)*lock;
	return pthread_mutex_unlock(plock);
}

