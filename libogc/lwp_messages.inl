#ifndef __MESSAGE_INL__
#define __MESSAGE_INL__

static __inline__ void _CORE_message_queue_Set_notify(CORE_message_queue_Control *mqueue,CORE_message_queue_Notify_Handler handler,void *arg)
{
	mqueue->notify_handler = handler;
	mqueue->notify_argument = arg;
}

static __inline__ u32 _CORE_message_queue_Is_priority(CORE_message_queue_Attributes *attr)
{
	return (attr->discipline==LWP_MQ_PRIORITY);
}

static __inline__ CORE_message_queue_Buffer_control* _CORE_message_queue_Allocate_message_buffer(CORE_message_queue_Control *mqueue)
{
	return (CORE_message_queue_Buffer_control*)_Chain_Get(&mqueue->Inactive_messages);
}

static __inline__ void _CORE_message_queue_Free_message_buffer(CORE_message_queue_Control *mqueue,CORE_message_queue_Buffer_control *msg)
{
	_Chain_Append(&mqueue->Inactive_messages,&msg->Node);
}

static __inline__ void _CORE_message_queue_Append(CORE_message_queue_Control *mqueue,CORE_message_queue_Buffer_control *msg)
{
#ifdef _LWPMQ_DEBUG
	printf("__lwpmq_msq_append(%p,%p,%p)\n",mqueue,&mqueue->inactive_msgs,msg);
#endif
	_Chain_Append(&mqueue->Pending_messages,&msg->Node);
}

static __inline__ void _CORE_message_queue_Prepend(CORE_message_queue_Control *mqueue,CORE_message_queue_Buffer_control *msg)
{
#ifdef _LWPMQ_DEBUG
	printf("__lwpmq_msq_prepend(%p,%p,%p)\n",mqueue,&mqueue->inactive_msgs,msg);
#endif
	_Chain_Prepend(&mqueue->Pending_messages,&msg->Node);
}

static __inline__ u32 _CORE_message_queue_Send(CORE_message_queue_Control *mqueue,u32 id,void *buffer,u32 size,u32 wait,u32 timeout)
{
	return _CORE_message_queue_Submit(mqueue,id,buffer,size,LWP_MQ_SEND_REQUEST,wait,timeout);
}

static __inline__ u32 _CORE_message_queue_Urgent(CORE_message_queue_Control *mqueue,void *buffer,u32 size,u32 id,u32 wait,u32 timeout)
{
	return _CORE_message_queue_Submit(mqueue,id,buffer,size,LWP_MQ_SEND_URGENT,wait,timeout);
}

static __inline__ CORE_message_queue_Buffer_control* _CORE_message_queue_Get_pending_message(CORE_message_queue_Control *mqueue)
{
	return (CORE_message_queue_Buffer_control*)_Chain_Get_unprotected(&mqueue->Pending_messages);
}

static __inline__ void _CORE_message_queue_Copy_buffer(void *dest,const void *src,u32 size)
{
	if(size==sizeof(u32)) *(u32*)dest = *(u32*)src;
	else memcpy(dest,src,size);
}

#endif
