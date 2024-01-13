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

typedef struct
{
	Objects_Control Object;
	CORE_message_queue_Control Message_queue;
} POSIX_Message_queue_Control;

Objects_Information _lwp_mqbox_objects;

void __lwp_mqbox_init()
{
	_Objects_Initialize_information(&_lwp_mqbox_objects,CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES,sizeof(POSIX_Message_queue_Control));
}

static __inline__ POSIX_Message_queue_Control* __lwp_mqbox_open(mqbox_t mbox)
{
	LWP_CHECK_MBOX(mbox);
	return (POSIX_Message_queue_Control*)_Objects_Get(&_lwp_mqbox_objects,LWP_OBJMASKID(mbox));
}

static __inline__ void __lwp_mqbox_free(POSIX_Message_queue_Control *mqbox)
{
	_Objects_Close(&_lwp_mqbox_objects,&mqbox->Object);
	_Objects_Free(&_lwp_mqbox_objects,&mqbox->Object);
}

static POSIX_Message_queue_Control* __lwp_mqbox_allocate()
{
	POSIX_Message_queue_Control *mqbox;

	_Thread_Disable_dispatch();
	mqbox = (POSIX_Message_queue_Control*)_Objects_Allocate(&_lwp_mqbox_objects);
	if(mqbox) {
		_Objects_Open(&_lwp_mqbox_objects,&mqbox->Object);
		return mqbox;
	}
	_Thread_Enable_dispatch();
	return NULL;
}

s32 MQ_Init(mqbox_t *mqbox,u32 count)
{
	CORE_message_queue_Attributes attr;
	POSIX_Message_queue_Control *ret = NULL;

	if(!mqbox) return -1;

	ret = __lwp_mqbox_allocate();
	if(!ret) return MQ_ERROR_TOOMANY;

	attr.discipline = CORE_MESSAGE_QUEUE_DISCIPLINES_FIFO;
	if(!_CORE_message_queue_Initialize(&ret->Message_queue,&attr,count,sizeof(mqmsg_t))) {
		__lwp_mqbox_free(ret);
		_Thread_Enable_dispatch();
		return MQ_ERROR_TOOMANY;
	}

	*mqbox = (mqbox_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_MBOX)|LWP_OBJMASKID(ret->Object.id));
	_Thread_Enable_dispatch();
	return MQ_ERROR_SUCCESSFUL;
}

void MQ_Close(mqbox_t mqbox)
{
	POSIX_Message_queue_Control *mbox;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return;

	_CORE_message_queue_Close(&mbox->Message_queue,0);
	_Thread_Enable_dispatch();

	__lwp_mqbox_free(mbox);
}

BOOL MQ_Send(mqbox_t mqbox,mqmsg_t msg,u32 flags)
{
	BOOL ret;
	POSIX_Message_queue_Control *mbox;
	u32 wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return FALSE;

	ret = FALSE;
	if(_CORE_message_queue_Submit(&mbox->Message_queue,mbox->Object.id,(void*)&msg,sizeof(mqmsg_t),CORE_MESSAGE_QUEUE_SEND_REQUEST,wait,RTEMS_NO_TIMEOUT)==CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL) ret = TRUE;
	_Thread_Enable_dispatch();

	return ret;
}

BOOL MQ_Receive(mqbox_t mqbox,mqmsg_t *msg,u32 flags)
{
	BOOL ret;
	POSIX_Message_queue_Control *mbox;
	u32 tmp,wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return FALSE;

	ret = FALSE;
	if(_CORE_message_queue_Seize(&mbox->Message_queue,mbox->Object.id,(void*)msg,&tmp,wait,RTEMS_NO_TIMEOUT)==CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL) ret = TRUE;
	_Thread_Enable_dispatch();

	return ret;
}

BOOL MQ_Jam(mqbox_t mqbox,mqmsg_t msg,u32 flags)
{
	BOOL ret;
	POSIX_Message_queue_Control *mbox;
	u32 wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return FALSE;

	ret = FALSE;
	if(_CORE_message_queue_Submit(&mbox->Message_queue,mbox->Object.id,(void*)&msg,sizeof(mqmsg_t),CORE_MESSAGE_QUEUE_URGENT_REQUEST,wait,RTEMS_NO_TIMEOUT)==CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL) ret = TRUE;
	_Thread_Enable_dispatch();

	return ret;
}
