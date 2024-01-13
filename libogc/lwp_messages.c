#include <stdlib.h>
#include "asm.h"
#include "lwp_messages.h"
#include "lwp_wkspace.h"

void _CORE_message_queue_Insert_message(CORE_message_queue_Control *the_message_queue,CORE_message_queue_Buffer_control *the_message,u32 submit_type)
{
	++the_message_queue->number_of_pending_messages;
	the_message->priority = submit_type;

#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Insert_message(%p,%p,%d)\n",the_message_queue,the_message,submit_type);
#endif

	switch(submit_type) {
		case LWP_MQ_SEND_REQUEST:
			_CORE_message_queue_Append(the_message_queue,the_message);
			break;
		case LWP_MQ_SEND_URGENT:
			_CORE_message_queue_Prepend(the_message_queue,the_message);
			break;
		default:
		{
			CORE_message_queue_Buffer_control *this_message;
			Chain_Node *the_node;
			Chain_Control *the_header;

			the_header = &the_message_queue->Pending_messages;
			the_node = the_header->first;
			while(!_Chain_Is_tail(the_header,the_node)) {
				this_message = (CORE_message_queue_Buffer_control*)the_node;
				if(this_message->priority<=the_message->priority) {
					the_node = the_node->next;
					continue;
				}
				break;
			}
			_Chain_Insert(the_node->previous,&the_message->Node);
		}
		break;
	}
	
	if(the_message_queue->number_of_pending_messages==1 && the_message_queue->notify_handler)
		the_message_queue->notify_handler(the_message_queue->notify_argument);
}

u32 _CORE_message_queue_Initialize(CORE_message_queue_Control *the_message_queue,CORE_message_queue_Attributes *the_message_queue_attributes,u32 maximum_pending_messages,u32 maximum_message_size)
{
	u32 allocated_message_size;
	u32 message_buffering_required;
	
#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Initialize(%p,%p,%d,%d)\n",the_message_queue,the_message_queue_attributes,maximum_pending_messages,maximum_message_size);
#endif
	the_message_queue->maximum_pending_messages = maximum_pending_messages;
	the_message_queue->number_of_pending_messages = 0;
	the_message_queue->maximum_message_size = maximum_message_size;
	_CORE_message_queue_Set_notify(the_message_queue,NULL,NULL);
	
	allocated_message_size = maximum_message_size;
	if(allocated_message_size&(sizeof(u32)-1))
		allocated_message_size = (allocated_message_size+sizeof(u32))&~(sizeof(u32)-1);
	
	message_buffering_required = maximum_pending_messages*(allocated_message_size+sizeof(CORE_message_queue_Buffer_control));
	the_message_queue->message_buffers = (CORE_message_queue_Buffer*)_Workspace_Allocate(message_buffering_required);

	if(!the_message_queue->message_buffers) return 0;

	_Chain_Initialize(&the_message_queue->Inactive_messages,the_message_queue->message_buffers,maximum_pending_messages,(allocated_message_size+sizeof(CORE_message_queue_Buffer_control)));
	_Chain_Initialize_empty(&the_message_queue->Pending_messages);
	_Thread_queue_Initialize(&the_message_queue->Wait_queue,_CORE_message_queue_Is_priority(the_message_queue_attributes)?THREAD_QUEUE_DISCIPLINE_PRIORITY:THREAD_QUEUE_DISCIPLINE_FIFO,LWP_STATES_WAITING_FOR_MESSAGE,LWP_MQ_STATUS_TIMEOUT);

	return 1;
}

u32 _CORE_message_queue_Seize(CORE_message_queue_Control *the_message_queue,u32 id,void *buffer,u32 *size,u32 wait,u64 timeout)
{
	u32 level;
	CORE_message_queue_Buffer_control *the_message;
	Thread_Control *executing,*the_thread;

	executing = _Thread_Executing;
	executing->Wait.return_code = LWP_MQ_STATUS_SUCCESSFUL;
#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Seize(%p,%d,%p,%p,%d,%d)\n",the_message_queue,id,buffer,size,wait,the_message_queue->num_pendingmsgs);
#endif
	
	_CPU_ISR_Disable(level);
	if(the_message_queue->number_of_pending_messages!=0) {
		--the_message_queue->number_of_pending_messages;
		the_message = _CORE_message_queue_Get_pending_message(the_message_queue);
		_CPU_ISR_Restore(level);
		
		*size = the_message->Contents.size;
		executing->Wait.count = the_message->priority;
		_CORE_message_queue_Copy_buffer(buffer,the_message->Contents.buffer,*size);

		the_thread = _Thread_queue_Dequeue(&the_message_queue->Wait_queue);
		if(!the_thread) {
			_CORE_message_queue_Free_message_buffer(the_message_queue,the_message);
			return LWP_MQ_STATUS_SUCCESSFUL;
		}

		the_message->priority = the_thread->Wait.count;
		the_message->Contents.size = (u32)the_thread->Wait.return_argument_1;
		_CORE_message_queue_Copy_buffer(the_message->Contents.buffer,the_thread->Wait.return_argument,the_message->Contents.size);
		
		_CORE_message_queue_Insert_message(the_message_queue,the_message,the_message->priority);
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	if(!wait) {
		_CPU_ISR_Restore(level);
		executing->Wait.return_code = LWP_MQ_STATUS_UNSATISFIED_NOWAIT;
		return LWP_MQ_STATUS_UNSATISFIED_NOWAIT;
	}

	_Thread_queue_Enter_critical_section(&the_message_queue->Wait_queue);
	executing->Wait.queue = &the_message_queue->Wait_queue;
	executing->Wait.id = id;
	executing->Wait.return_argument = (void*)buffer;
	executing->Wait.return_argument_1 = (void*)size;
	_CPU_ISR_Restore(level);

	_Thread_queue_Enqueue(&the_message_queue->Wait_queue,timeout);
	return LWP_MQ_STATUS_SUCCESSFUL;
}

u32 _CORE_message_queue_Submit(CORE_message_queue_Control *the_message_queue,u32 id,void *buffer,u32 size,u32 submit_type,u32 wait,u64 timeout)
{
	u32 level;
	Thread_Control *the_thread;
	CORE_message_queue_Buffer_control *the_message;

#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Submit(%p,%p,%d,%d,%d,%d)\n",the_message_queue,buffer,size,id,submit_type,wait);
#endif
	if(size>the_message_queue->maximum_message_size)
		return LWP_MQ_STATUS_INVALID_SIZE;

	if(the_message_queue->number_of_pending_messages==0) {
		the_thread = _Thread_queue_Dequeue(&the_message_queue->Wait_queue);
		if(the_thread) {
			_CORE_message_queue_Copy_buffer(the_thread->Wait.return_argument,buffer,size);
			*(u32*)the_thread->Wait.return_argument_1 = size;
			the_thread->Wait.count = submit_type;
			return LWP_MQ_STATUS_SUCCESSFUL;
		}
	}

	if(the_message_queue->number_of_pending_messages<the_message_queue->maximum_pending_messages) {
		the_message = _CORE_message_queue_Allocate_message_buffer(the_message_queue);
		if(!the_message) return LWP_MQ_STATUS_UNSATISFIED;

		_CORE_message_queue_Copy_buffer(the_message->Contents.buffer,buffer,size);
		the_message->Contents.size = size;
		the_message->priority = submit_type;
		_CORE_message_queue_Insert_message(the_message_queue,the_message,submit_type);
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	if(!wait) return LWP_MQ_STATUS_TOO_MANY;
	if(_ISR_Is_in_progress()) return LWP_MQ_STATUS_UNSATISFIED;

	{
		Thread_Control *exec = _Thread_Executing;

		_CPU_ISR_Disable(level);
		_Thread_queue_Enter_critical_section(&the_message_queue->Wait_queue);
		exec->Wait.queue = &the_message_queue->Wait_queue;
		exec->Wait.id = id;
		exec->Wait.return_argument = (void*)buffer;
		exec->Wait.return_argument_1 = (void*)size;
		exec->Wait.count = submit_type;
		_CPU_ISR_Restore(level);
		
		_Thread_queue_Enqueue(&the_message_queue->Wait_queue,timeout);
	}
	return LWP_MQ_STATUS_UNSATISFIED_WAIT;
}

u32 _CORE_message_queue_Broadcast(CORE_message_queue_Control *the_message_queue,void *buffer,u32 size,u32 id,u32 *count)
{
	Thread_Control *the_thread;
	u32 number_broadcasted;
	Thread_Wait_information *waitp;
	u32 constrained_size;
#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Broadcast(%p,%p,%d,%d,%p)\n",the_message_queue,buffer,size,id,count);
#endif
	if(the_message_queue->number_of_pending_messages!=0) {
		*count = 0;
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	number_broadcasted = 0;
	while((the_thread=_Thread_queue_Dequeue(&the_message_queue->Wait_queue))) {
		waitp = &the_thread->Wait;
		++number_broadcasted;

		constrained_size = size;
		if(size>the_message_queue->maximum_message_size)
			constrained_size = the_message_queue->maximum_message_size;

		_CORE_message_queue_Copy_buffer(waitp->return_argument,buffer,constrained_size);
		*(u32*)waitp->return_argument = size;
	}
	*count = number_broadcasted;
	return LWP_MQ_STATUS_SUCCESSFUL;
}

void _CORE_message_queue_Close(CORE_message_queue_Control *the_message_queue,u32 status)
{
	_Thread_queue_Flush(&the_message_queue->Wait_queue,status);
	_CORE_message_queue_Flush_support(the_message_queue);
	_Workspace_Free(the_message_queue->message_buffers);
}

u32 _CORE_message_queue_Flush(CORE_message_queue_Control *the_message_queue)
{
	if(the_message_queue->number_of_pending_messages!=0)
		return _CORE_message_queue_Flush_support(the_message_queue);
	else
		return 0;
}

u32 _CORE_message_queue_Flush_support(CORE_message_queue_Control *the_message_queue)
{
	u32 level;
	Chain_Node *inactive_first;
	Chain_Node *message_queue_first;
	Chain_Node *message_queue_last;
	u32 count;

	_CPU_ISR_Disable(level);

	inactive_first = the_message_queue->Inactive_messages.first;
	message_queue_first = the_message_queue->Pending_messages.first;
	message_queue_last = the_message_queue->Pending_messages.last;

	the_message_queue->Inactive_messages.first = message_queue_first;
	message_queue_last->next = inactive_first;
	inactive_first->previous = message_queue_last;
	message_queue_first->previous = _Chain_Head(&the_message_queue->Inactive_messages);
	
	_Chain_Initialize_empty(&the_message_queue->Pending_messages);

	count = the_message_queue->number_of_pending_messages;
	the_message_queue->number_of_pending_messages = 0;

	_CPU_ISR_Restore(level);
	return count;
}

void _CORE_message_queue_Flush_waiting_threads(CORE_message_queue_Control *the_message_queue)
{
	_Thread_queue_Flush(&the_message_queue->Wait_queue,LWP_MQ_STATUS_UNSATISFIED_NOWAIT);
}
