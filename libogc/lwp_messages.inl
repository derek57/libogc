#ifndef __MESSAGE_INL__
#define __MESSAGE_INL__

static __inline__ void _CORE_message_queue_Set_notify(mq_cntrl *mqueue,mq_notifyhandler handler,void *arg)
{
	mqueue->notify_handler = handler;
	mqueue->notify_arg = arg;
}

static __inline__ u32 _CORE_message_queue_Is_priority(mq_attr *attr)
{
	return (attr->mode==LWP_MQ_PRIORITY);
}

static __inline__ mq_buffercntrl* _CORE_message_queue_Allocate_message_buffer(mq_cntrl *mqueue)
{
	return (mq_buffercntrl*)_Chain_Get(&mqueue->inactive_msgs);
}

static __inline__ void _CORE_message_queue_Free_message_buffer(mq_cntrl *mqueue,mq_buffercntrl *msg)
{
	_Chain_Append(&mqueue->inactive_msgs,&msg->node);
}

static __inline__ void _CORE_message_queue_Append(mq_cntrl *mqueue,mq_buffercntrl *msg)
{
#ifdef _LWPMQ_DEBUG
	printf("__lwpmq_msq_append(%p,%p,%p)\n",mqueue,&mqueue->inactive_msgs,msg);
#endif
	_Chain_Append(&mqueue->pending_msgs,&msg->node);
}

static __inline__ void _CORE_message_queue_Prepend(mq_cntrl *mqueue,mq_buffercntrl *msg)
{
#ifdef _LWPMQ_DEBUG
	printf("__lwpmq_msq_prepend(%p,%p,%p)\n",mqueue,&mqueue->inactive_msgs,msg);
#endif
	_Chain_Prepend(&mqueue->pending_msgs,&msg->node);
}

static __inline__ u32 _CORE_message_queue_Send(mq_cntrl *mqueue,u32 id,void *buffer,u32 size,u32 wait,u32 timeout)
{
	return _CORE_message_queue_Submit(mqueue,id,buffer,size,LWP_MQ_SEND_REQUEST,wait,timeout);
}

static __inline__ u32 _CORE_message_queue_Urgent(mq_cntrl *mqueue,void *buffer,u32 size,u32 id,u32 wait,u32 timeout)
{
	return _CORE_message_queue_Submit(mqueue,id,buffer,size,LWP_MQ_SEND_URGENT,wait,timeout);
}

static __inline__ mq_buffercntrl* _CORE_message_queue_Get_pending_message(mq_cntrl *mqueue)
{
	return (mq_buffercntrl*)_Chain_Get_unprotected(&mqueue->pending_msgs);
}

static __inline__ void _CORE_message_queue_Copy_buffer(void *dest,const void *src,u32 size)
{
	if(size==sizeof(u32)) *(u32*)dest = *(u32*)src;
	else memcpy(dest,src,size);
}

#endif
