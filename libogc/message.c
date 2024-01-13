/*-------------------------------------------------------------

message.c -- Thread subsystem II

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


#include <asm.h>
#include <stdlib.h>
#include <lwp_messages.h>
#include <lwp_objmgr.h>
#include <lwp_config.h>
#include "message.h"

#define LWP_OBJTYPE_MBOX			6

#define LWP_CHECK_MBOX(hndl)		\
{									\
	if(((hndl)==MQ_BOX_NULL) || (LWP_OBJTYPE(hndl)!=LWP_OBJTYPE_MBOX))	\
		return NULL;				\
}

typedef struct _mqbox_st
{
	Objects_Control object;
	CORE_message_queue_Control mqueue;
} mqbox_st;

Objects_Information _lwp_mqbox_objects;

void __lwp_mqbox_init()
{
	_Objects_Initialize_information(&_lwp_mqbox_objects,CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES,sizeof(mqbox_st));
}

static __inline__ mqbox_st* __lwp_mqbox_open(mqbox_t mbox)
{
	LWP_CHECK_MBOX(mbox);
	return (mqbox_st*)_Objects_Get(&_lwp_mqbox_objects,LWP_OBJMASKID(mbox));
}

static __inline__ void __lwp_mqbox_free(mqbox_st *mqbox)
{
	_Objects_Close(&_lwp_mqbox_objects,&mqbox->object);
	_Objects_Free(&_lwp_mqbox_objects,&mqbox->object);
}

static mqbox_st* __lwp_mqbox_allocate()
{
	mqbox_st *mqbox;

	_Thread_Disable_dispatch();
	mqbox = (mqbox_st*)_Objects_Allocate(&_lwp_mqbox_objects);
	if(mqbox) {
		_Objects_Open(&_lwp_mqbox_objects,&mqbox->object);
		return mqbox;
	}
	_Thread_Enable_dispatch();
	return NULL;
}

s32 MQ_Init(mqbox_t *mqbox,u32 count)
{
	CORE_message_queue_Attributes attr;
	mqbox_st *ret = NULL;

	if(!mqbox) return -1;

	ret = __lwp_mqbox_allocate();
	if(!ret) return MQ_ERROR_TOOMANY;

	attr.discipline = CORE_MESSAGE_QUEUE_DISCIPLINES_FIFO;
	if(!_CORE_message_queue_Initialize(&ret->mqueue,&attr,count,sizeof(mqmsg_t))) {
		__lwp_mqbox_free(ret);
		_Thread_Enable_dispatch();
		return MQ_ERROR_TOOMANY;
	}

	*mqbox = (mqbox_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_MBOX)|LWP_OBJMASKID(ret->object.id));
	_Thread_Enable_dispatch();
	return MQ_ERROR_SUCCESSFUL;
}

void MQ_Close(mqbox_t mqbox)
{
	mqbox_st *mbox;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return;

	_CORE_message_queue_Close(&mbox->mqueue,0);
	_Thread_Enable_dispatch();

	__lwp_mqbox_free(mbox);
}

BOOL MQ_Send(mqbox_t mqbox,mqmsg_t msg,u32 flags)
{
	BOOL ret;
	mqbox_st *mbox;
	u32 wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return FALSE;

	ret = FALSE;
	if(_CORE_message_queue_Submit(&mbox->mqueue,mbox->object.id,(void*)&msg,sizeof(mqmsg_t),CORE_MESSAGE_QUEUE_SEND_REQUEST,wait,LWP_THREADQ_NOTIMEOUT)==CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL) ret = TRUE;
	_Thread_Enable_dispatch();

	return ret;
}

BOOL MQ_Receive(mqbox_t mqbox,mqmsg_t *msg,u32 flags)
{
	BOOL ret;
	mqbox_st *mbox;
	u32 tmp,wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return FALSE;

	ret = FALSE;
	if(_CORE_message_queue_Seize(&mbox->mqueue,mbox->object.id,(void*)msg,&tmp,wait,LWP_THREADQ_NOTIMEOUT)==CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL) ret = TRUE;
	_Thread_Enable_dispatch();

	return ret;
}

BOOL MQ_Jam(mqbox_t mqbox,mqmsg_t msg,u32 flags)
{
	BOOL ret;
	mqbox_st *mbox;
	u32 wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return FALSE;

	ret = FALSE;
	if(_CORE_message_queue_Submit(&mbox->mqueue,mbox->object.id,(void*)&msg,sizeof(mqmsg_t),CORE_MESSAGE_QUEUE_URGENT_REQUEST,wait,LWP_THREADQ_NOTIMEOUT)==CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL) ret = TRUE;
	_Thread_Enable_dispatch();

	return ret;
}
