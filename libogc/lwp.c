/*-------------------------------------------------------------

lwp.c -- Thread subsystem I

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
#include "processor.h"
#include "lwp_threadq.h"
#include "lwp_threads.h"
#include "lwp_wkspace.h"
#include "lwp_objmgr.h"
#include "lwp_config.h"
#include "lwp.h"

#define LWP_OBJTYPE_THREAD			1
#define LWP_OBJTYPE_TQUEUE			2

#define LWP_CHECK_THREAD(hndl)		\
{									\
	if(((hndl)==LWP_THREAD_NULL) || (LWP_OBJTYPE(hndl)!=LWP_OBJTYPE_THREAD))	\
		return NULL;				\
}

#define LWP_CHECK_TQUEUE(hndl)		\
{									\
	if(((hndl)==LWP_TQUEUE_NULL) || (LWP_OBJTYPE(hndl)!=LWP_OBJTYPE_TQUEUE))	\
		return NULL;				\
}

typedef struct _tqueue_st {
	Objects_Control object;
	Thread_queue_Control tqueue;
} tqueue_st;

Objects_Information _lwp_thr_objects;
Objects_Information _lwp_tqueue_objects;

extern int __crtmain();

extern u8 __stack_addr[],__stack_end[];

RTEMS_INLINE_ROUTINE u32 __lwp_priotocore(u32 prio)
{
	return (255 - prio);
}

RTEMS_INLINE_ROUTINE Thread_Control* __lwp_cntrl_open(lwp_t thr_id)
{
	LWP_CHECK_THREAD(thr_id);
	return (Thread_Control*)_Objects_Get(&_lwp_thr_objects,LWP_OBJMASKID(thr_id));
}

RTEMS_INLINE_ROUTINE tqueue_st* __lwp_tqueue_open(lwpq_t tqueue)
{
	LWP_CHECK_TQUEUE(tqueue);
	return (tqueue_st*)_Objects_Get(&_lwp_tqueue_objects,LWP_OBJMASKID(tqueue));
}

static Thread_Control* __lwp_cntrl_allocate()
{
	Thread_Control *thethread;
	
	_Thread_Disable_dispatch();
	thethread = (Thread_Control*)_Objects_Allocate(&_lwp_thr_objects);
	if(thethread) {
		_Objects_Open(&_lwp_thr_objects,&thethread->Object);
		return thethread;
	}
	_Thread_Enable_dispatch();
	return NULL;
}

static tqueue_st* __lwp_tqueue_allocate()
{
	tqueue_st *tqueue;

	_Thread_Disable_dispatch();
	tqueue = (tqueue_st*)_Objects_Allocate(&_lwp_tqueue_objects);
	if(tqueue) {
		_Objects_Open(&_lwp_tqueue_objects,&tqueue->object);
		return tqueue;
	}
	_Thread_Enable_dispatch();
	return NULL;
}

RTEMS_INLINE_ROUTINE void __lwp_cntrl_free(Thread_Control *thethread)
{
	_Objects_Close(&_lwp_thr_objects,&thethread->Object);
	_Objects_Free(&_lwp_thr_objects,&thethread->Object);
}

RTEMS_INLINE_ROUTINE void __lwp_tqueue_free(tqueue_st *tq)
{
	_Objects_Close(&_lwp_tqueue_objects,&tq->object);
	_Objects_Free(&_lwp_tqueue_objects,&tq->object);
}

static void* _Thread_Idle_body(void *arg)
{
	while(1);
	return 0;
}

void __lwp_sysinit()
{
	_Objects_Initialize_information(&_lwp_thr_objects,CONFIGURE_MAXIMUM_POSIX_THREADS,sizeof(Thread_Control));
	_Objects_Initialize_information(&_lwp_tqueue_objects,CONFIGURE_MAXIMUM_POSIX_QUEUED_SIGNALS,sizeof(tqueue_st));

	// create idle thread, is needed iff all threads are locked on a queue
	_Thread_Idle = (Thread_Control*)_Objects_Allocate(&_lwp_thr_objects);
	_Thread_Initialize(_Thread_Idle,NULL,0,255,0,TRUE);
	_Thread_Executing = _Thread_Heir = _Thread_Idle;
	_Thread_Start(_Thread_Idle,_Thread_Idle_body,NULL);
	_Objects_Open(&_lwp_thr_objects,&_Thread_Idle->Object);

	// create main thread, as this is our entry point
	// for every GC application.
	_thr_main = (Thread_Control*)_Objects_Allocate(&_lwp_thr_objects);
	_Thread_Initialize(_thr_main,__stack_end,((u32)__stack_addr-(u32)__stack_end),191,0,TRUE);
	_Thread_Start(_thr_main,(void*)__crtmain,NULL);
	_Objects_Open(&_lwp_thr_objects,&_thr_main->Object);
}

BOOL __lwp_thread_isalive(lwp_t thr_id)
{
	if(thr_id==LWP_THREAD_NULL || LWP_OBJTYPE(thr_id)!=LWP_OBJTYPE_THREAD) return FALSE;

	Thread_Control *thethread = (Thread_Control*)_Objects_Get_no_protection(&_lwp_thr_objects,LWP_OBJMASKID(thr_id));
	
	if(thethread) {  
		u32 *stackbase = thethread->stack;
		if(stackbase[0]==0xDEADBABE && !_States_Is_dormant(thethread->current_state) && !_States_Is_Transient(thethread->current_state))
			return TRUE;
	}
	
	return FALSE;
}

lwp_t pthread_self()
{
	return _Thread_Executing->Object.id;
}

BOOL __lwp_thread_exists(lwp_t thr_id)
{
	if(thr_id==LWP_THREAD_NULL || LWP_OBJTYPE(thr_id)!=LWP_OBJTYPE_THREAD) return FALSE;
	return (_Objects_Get_no_protection(&_lwp_thr_objects,LWP_OBJMASKID(thr_id))!=NULL);
}

Context_Control* __lwp_thread_context(lwp_t thr_id)
{
	Thread_Control *thethread;
	Context_Control *pctx = NULL;

	LWP_CHECK_THREAD(thr_id);
	thethread = (Thread_Control*)_Objects_Get_no_protection(&_lwp_thr_objects,LWP_OBJMASKID(thr_id));
	if(thethread) pctx = &thethread->Registers;

	return pctx;
}

s32 LWP_CreateThread(lwp_t *thethread,void* (*entry)(void *),void *arg,void *stackbase,u32 stack_size,u8 prio)
{
	u32 status;
	Thread_Control *lwp_thread;
	
	if(!thethread || !entry) return -1;

	lwp_thread = __lwp_cntrl_allocate();
	if(!lwp_thread) return -1;

	status = _Thread_Initialize(lwp_thread,stackbase,stack_size,__lwp_priotocore(prio),0,TRUE);
	if(!status) {
		__lwp_cntrl_free(lwp_thread);
		_Thread_Enable_dispatch();
		return -1;
	}
	
	status = _Thread_Start(lwp_thread,entry,arg);
	if(!status) {
		__lwp_cntrl_free(lwp_thread);
		_Thread_Enable_dispatch();
		return -1;
	}

	*thethread = (lwp_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_THREAD)|LWP_OBJMASKID(lwp_thread->Object.id));
	_Thread_Enable_dispatch();

	return 0;
}

s32 LWP_SuspendThread(lwp_t thethread)
{
	Thread_Control *lwp_thread;

	lwp_thread = __lwp_cntrl_open(thethread);
	if(!lwp_thread) return -1;

	if(!_States_Is_suspended(lwp_thread->current_state)) {
		_Thread_Suspend(lwp_thread);
		_Thread_Enable_dispatch();
		return LWP_SUCCESSFUL;
	}
	_Thread_Enable_dispatch();
	return LWP_ALREADY_SUSPENDED;
}

s32 LWP_ResumeThread(lwp_t thethread)
{
	Thread_Control *lwp_thread;

	lwp_thread = __lwp_cntrl_open(thethread);
	if(!lwp_thread) return -1;

	if(_States_Is_suspended(lwp_thread->current_state)) {
		_Thread_Resume(lwp_thread,TRUE);
		_Thread_Enable_dispatch();
		return LWP_SUCCESSFUL;
	}
	_Thread_Enable_dispatch();
	return LWP_NOT_SUSPENDED;
}

lwp_t LWP_GetSelf()
{
	lwp_t ret;

	_Thread_Disable_dispatch();
	ret = (lwp_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_THREAD)|LWP_OBJMASKID(_Thread_Executing->Object.id));
	_Thread_Unnest_dispatch();

	return ret;
}

void LWP_SetThreadPriority(lwp_t thethread,u32 prio)
{
	Thread_Control *lwp_thread;

	if(thethread==LWP_THREAD_NULL) thethread = LWP_GetSelf();

	lwp_thread = __lwp_cntrl_open(thethread);
	if(!lwp_thread) return;

	_Thread_Change_priority(lwp_thread,__lwp_priotocore(prio),TRUE);
	_Thread_Enable_dispatch();
}

void LWP_YieldThread()
{
	_Thread_Disable_dispatch();
	_Thread_Yield_processor();
	_Thread_Enable_dispatch();
}

void LWP_Reschedule(u32 prio)
{
	_Thread_Disable_dispatch();
	_Thread_Rotate_Ready_Queue(prio);
	_Thread_Enable_dispatch();
}

BOOL LWP_ThreadIsSuspended(lwp_t thethread)
{
	BOOL state;
	Thread_Control *lwp_thread;

	lwp_thread = __lwp_cntrl_open(thethread);
  	if(!lwp_thread) return FALSE;
	
	state = (_States_Is_suspended(lwp_thread->current_state) ? TRUE : FALSE);

	_Thread_Enable_dispatch();
	return state;
}


s32 LWP_JoinThread(lwp_t thethread,void **value_ptr)
{
	u32 level;
	void *return_ptr;
	Thread_Control *exec,*lwp_thread;
	
	lwp_thread = __lwp_cntrl_open(thethread);
	if(!lwp_thread) return 0;

	if(_Thread_Is_executing(lwp_thread)) {
		_Thread_Enable_dispatch();
		return EDEADLK;			//EDEADLK
	}

	exec = _Thread_Executing;
	_CPU_ISR_Disable(level);
	_Thread_queue_Enter_critical_section(&lwp_thread->Join_List);
	exec->Wait.return_code = 0;
	exec->Wait.return_argument_1 = NULL;
	exec->Wait.return_argument = (void*)&return_ptr;
	exec->Wait.queue = &lwp_thread->Join_List;
	exec->Wait.id = thethread;
	_CPU_ISR_Restore(level);
	_Thread_queue_Enqueue(&lwp_thread->Join_List,WATCHDOG_NO_TIMEOUT);
	_Thread_Enable_dispatch();

	if(value_ptr) *value_ptr = return_ptr;
	return 0;
}

s32 LWP_InitQueue(lwpq_t *thequeue)
{
	tqueue_st *tq;

	if(!thequeue) return -1;

	tq = __lwp_tqueue_allocate();
	if(!tq) return -1;

	_Thread_queue_Initialize(&tq->tqueue,THREAD_QUEUE_DISCIPLINE_FIFO,STATES_WAITING_ON_THREAD_QUEUE,0);

	*thequeue = (lwpq_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_TQUEUE)|LWP_OBJMASKID(tq->object.id));
	_Thread_Enable_dispatch();

	return 0;
}

void LWP_CloseQueue(lwpq_t thequeue)
{
	Thread_Control *thethread;
	tqueue_st *tq = (tqueue_st*)thequeue;

	tq = __lwp_tqueue_open(thequeue);
	if(!tq) return;
	
	do {
		thethread = _Thread_queue_Dequeue(&tq->tqueue);
	} while(thethread);
	_Thread_Enable_dispatch();

	__lwp_tqueue_free(tq);
	return;
}

s32 LWP_ThreadSleep(lwpq_t thequeue)
{
	u32 level;
	tqueue_st *tq;
	Thread_Control *exec = NULL;

	tq = __lwp_tqueue_open(thequeue);
	if(!tq) return -1;

	exec = _Thread_Executing;
	_CPU_ISR_Disable(level);
	_Thread_queue_Enter_critical_section(&tq->tqueue);
	exec->Wait.return_code = 0;
	exec->Wait.return_argument = NULL;
	exec->Wait.return_argument_1 = NULL;
	exec->Wait.queue = &tq->tqueue;
	exec->Wait.id = thequeue;
	_CPU_ISR_Restore(level);
	_Thread_queue_Enqueue(&tq->tqueue,RTEMS_NO_TIMEOUT);
	_Thread_Enable_dispatch();
	return 0;
}

void LWP_ThreadBroadcast(lwpq_t thequeue)
{
	tqueue_st *tq;
	Thread_Control *thethread;

	tq = __lwp_tqueue_open(thequeue);
	if(!tq) return;
	
	do {
		thethread = _Thread_queue_Dequeue(&tq->tqueue);
	} while(thethread);
	_Thread_Enable_dispatch();
}

void LWP_ThreadSignal(lwpq_t thequeue)
{
	tqueue_st *tq;

	tq = __lwp_tqueue_open(thequeue);
	if(!tq) return;

	_Thread_queue_Dequeue(&tq->tqueue);
	_Thread_Enable_dispatch();
}
