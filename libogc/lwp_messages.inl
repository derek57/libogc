#ifndef __MESSAGE_INL__
#define __MESSAGE_INL__

RTEMS_INLINE_ROUTINE void _CORE_message_queue_Set_notify(CORE_message_queue_Control *the_message_queue,CORE_message_queue_Notify_Handler the_handler,void *the_argument)
{
	the_message_queue->notify_handler = the_handler;
	the_message_queue->notify_argument = the_argument;
}

RTEMS_INLINE_ROUTINE u32 _CORE_message_queue_Is_priority(CORE_message_queue_Attributes *the_attribute)
{
	return (the_attribute->discipline==CORE_MESSAGE_QUEUE_DISCIPLINES_PRIORITY);
}

RTEMS_INLINE_ROUTINE CORE_message_queue_Buffer_control* _CORE_message_queue_Allocate_message_buffer(CORE_message_queue_Control *the_message_queue)
{
	return (CORE_message_queue_Buffer_control*)_Chain_Get(&the_message_queue->Inactive_messages);
}

RTEMS_INLINE_ROUTINE void _CORE_message_queue_Free_message_buffer(CORE_message_queue_Control *the_message_queue,CORE_message_queue_Buffer_control *the_message)
{
	_Chain_Append(&the_message_queue->Inactive_messages,&the_message->Node);
}

RTEMS_INLINE_ROUTINE void _CORE_message_queue_Append(CORE_message_queue_Control *the_message_queue,CORE_message_queue_Buffer_control *the_message)
{
#ifdef _LWPMQ_DEBUG
	printf("__lwpmq_msq_append(%p,%p,%p)\n",the_message_queue,&the_message_queue->inactive_msgs,the_message);
#endif
	_Chain_Append(&the_message_queue->Pending_messages,&the_message->Node);
}

RTEMS_INLINE_ROUTINE void _CORE_message_queue_Prepend(CORE_message_queue_Control *the_message_queue,CORE_message_queue_Buffer_control *the_message)
{
#ifdef _LWPMQ_DEBUG
	printf("__lwpmq_msq_prepend(%p,%p,%p)\n",the_message_queue,&the_message_queue->inactive_msgs,the_message);
#endif
	_Chain_Prepend(&the_message_queue->Pending_messages,&the_message->Node);
}

RTEMS_INLINE_ROUTINE u32 _CORE_message_queue_Send(CORE_message_queue_Control *the_message_queue,u32 id,void *buffer,u32 size,u32 wait,u32 timeout)
{
	return _CORE_message_queue_Submit(the_message_queue,id,buffer,size,CORE_MESSAGE_QUEUE_SEND_REQUEST,wait,timeout);
}

RTEMS_INLINE_ROUTINE u32 _CORE_message_queue_Urgent(CORE_message_queue_Control *the_message_queue,void *buffer,u32 size,u32 id,u32 wait,u32 timeout)
{
	return _CORE_message_queue_Submit(the_message_queue,id,buffer,size,CORE_MESSAGE_QUEUE_URGENT_REQUEST,wait,timeout);
}

RTEMS_INLINE_ROUTINE CORE_message_queue_Buffer_control* _CORE_message_queue_Get_pending_message(CORE_message_queue_Control *the_message_queue)
{
	return (CORE_message_queue_Buffer_control*)_Chain_Get_unprotected(&the_message_queue->Pending_messages);
}

RTEMS_INLINE_ROUTINE void _CORE_message_queue_Copy_buffer(void *destination,const void *source,u32 size)
{
	if(size==sizeof(u32)) *(u32*)destination = *(u32*)source;
	else memcpy(destination,source,size);
}

#endif
