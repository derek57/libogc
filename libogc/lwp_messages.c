#include <stdlib.h>
#include "asm.h"
#include "lwp_messages.h"
#include "lwp_wkspace.h"

void _CORE_message_queue_Insert_message(CORE_message_queue_Control *mqueue,CORE_message_queue_Buffer_control *msg,u32 type)
{
	++mqueue->number_of_pending_messages;
	msg->priority = type;

#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Insert_message(%p,%p,%d)\n",mqueue,msg,type);
#endif

	switch(type) {
		case LWP_MQ_SEND_REQUEST:
			_CORE_message_queue_Append(mqueue,msg);
			break;
		case LWP_MQ_SEND_URGENT:
			_CORE_message_queue_Prepend(mqueue,msg);
			break;
		default:
		{
			CORE_message_queue_Buffer_control *tmsg;
			Chain_Node *node;
			Chain_Control *header;

			header = &mqueue->Pending_messages;
			node = header->first;
			while(!_Chain_Is_tail(header,node)) {
				tmsg = (CORE_message_queue_Buffer_control*)node;
				if(tmsg->priority<=msg->priority) {
					node = node->next;
					continue;
				}
				break;
			}
			_Chain_Insert(node->previous,&msg->Node);
		}
		break;
	}
	
	if(mqueue->number_of_pending_messages==1 && mqueue->notify_handler)
		mqueue->notify_handler(mqueue->notify_argument);
}

u32 _CORE_message_queue_Initialize(CORE_message_queue_Control *mqueue,CORE_message_queue_Attributes *attrs,u32 max_pendingmsgs,u32 max_msgsize)
{
	u32 alloc_msgsize;
	u32 buffering_req;
	
#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Initialize(%p,%p,%d,%d)\n",mqueue,attrs,max_pendingmsgs,max_msgsize);
#endif
	mqueue->maximum_pending_messages = max_pendingmsgs;
	mqueue->number_of_pending_messages = 0;
	mqueue->maximum_message_size = max_msgsize;
	_CORE_message_queue_Set_notify(mqueue,NULL,NULL);
	
	alloc_msgsize = max_msgsize;
	if(alloc_msgsize&(sizeof(u32)-1))
		alloc_msgsize = (alloc_msgsize+sizeof(u32))&~(sizeof(u32)-1);
	
	buffering_req = max_pendingmsgs*(alloc_msgsize+sizeof(CORE_message_queue_Buffer_control));
	mqueue->message_buffers = (CORE_message_queue_Buffer*)_Workspace_Allocate(buffering_req);

	if(!mqueue->message_buffers) return 0;

	_Chain_Initialize(&mqueue->Inactive_messages,mqueue->message_buffers,max_pendingmsgs,(alloc_msgsize+sizeof(CORE_message_queue_Buffer_control)));
	_Chain_Initialize_empty(&mqueue->Pending_messages);
	_Thread_queue_Initialize(&mqueue->Wait_queue,_CORE_message_queue_Is_priority(attrs)?LWP_THREADQ_MODEPRIORITY:LWP_THREADQ_MODEFIFO,LWP_STATES_WAITING_FOR_MESSAGE,LWP_MQ_STATUS_TIMEOUT);

	return 1;
}

u32 _CORE_message_queue_Seize(CORE_message_queue_Control *mqueue,u32 id,void *buffer,u32 *size,u32 wait,u64 timeout)
{
	u32 level;
	CORE_message_queue_Buffer_control *msg;
	Thread_Control *exec,*thread;

	exec = _thr_executing;
	exec->Wait.return_code = LWP_MQ_STATUS_SUCCESSFUL;
#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Seize(%p,%d,%p,%p,%d,%d)\n",mqueue,id,buffer,size,wait,mqueue->num_pendingmsgs);
#endif
	
	_CPU_ISR_Disable(level);
	if(mqueue->number_of_pending_messages!=0) {
		--mqueue->number_of_pending_messages;
		msg = _CORE_message_queue_Get_pending_message(mqueue);
		_CPU_ISR_Restore(level);
		
		*size = msg->Contents.size;
		exec->Wait.count = msg->priority;
		_CORE_message_queue_Copy_buffer(buffer,msg->Contents.buffer,*size);

		thread = _Thread_queue_Dequeue(&mqueue->Wait_queue);
		if(!thread) {
			_CORE_message_queue_Free_message_buffer(mqueue,msg);
			return LWP_MQ_STATUS_SUCCESSFUL;
		}

		msg->priority = thread->Wait.count;
		msg->Contents.size = (u32)thread->Wait.return_argument_1;
		_CORE_message_queue_Copy_buffer(msg->Contents.buffer,thread->Wait.return_argument,msg->Contents.size);
		
		_CORE_message_queue_Insert_message(mqueue,msg,msg->priority);
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	if(!wait) {
		_CPU_ISR_Restore(level);
		exec->Wait.return_code = LWP_MQ_STATUS_UNSATISFIED_NOWAIT;
		return LWP_MQ_STATUS_UNSATISFIED_NOWAIT;
	}

	_Thread_queue_Enter_critical_section(&mqueue->Wait_queue);
	exec->Wait.queue = &mqueue->Wait_queue;
	exec->Wait.id = id;
	exec->Wait.return_argument = (void*)buffer;
	exec->Wait.return_argument_1 = (void*)size;
	_CPU_ISR_Restore(level);

	_Thread_queue_Enqueue(&mqueue->Wait_queue,timeout);
	return LWP_MQ_STATUS_SUCCESSFUL;
}

u32 _CORE_message_queue_Submit(CORE_message_queue_Control *mqueue,u32 id,void *buffer,u32 size,u32 type,u32 wait,u64 timeout)
{
	u32 level;
	Thread_Control *thread;
	CORE_message_queue_Buffer_control *msg;

#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Submit(%p,%p,%d,%d,%d,%d)\n",mqueue,buffer,size,id,type,wait);
#endif
	if(size>mqueue->maximum_message_size)
		return LWP_MQ_STATUS_INVALID_SIZE;

	if(mqueue->number_of_pending_messages==0) {
		thread = _Thread_queue_Dequeue(&mqueue->Wait_queue);
		if(thread) {
			_CORE_message_queue_Copy_buffer(thread->Wait.return_argument,buffer,size);
			*(u32*)thread->Wait.return_argument_1 = size;
			thread->Wait.count = type;
			return LWP_MQ_STATUS_SUCCESSFUL;
		}
	}

	if(mqueue->number_of_pending_messages<mqueue->maximum_pending_messages) {
		msg = _CORE_message_queue_Allocate_message_buffer(mqueue);
		if(!msg) return LWP_MQ_STATUS_UNSATISFIED;

		_CORE_message_queue_Copy_buffer(msg->Contents.buffer,buffer,size);
		msg->Contents.size = size;
		msg->priority = type;
		_CORE_message_queue_Insert_message(mqueue,msg,type);
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	if(!wait) return LWP_MQ_STATUS_TOO_MANY;
	if(_ISR_Is_in_progress()) return LWP_MQ_STATUS_UNSATISFIED;

	{
		Thread_Control *exec = _thr_executing;

		_CPU_ISR_Disable(level);
		_Thread_queue_Enter_critical_section(&mqueue->Wait_queue);
		exec->Wait.queue = &mqueue->Wait_queue;
		exec->Wait.id = id;
		exec->Wait.return_argument = (void*)buffer;
		exec->Wait.return_argument_1 = (void*)size;
		exec->Wait.count = type;
		_CPU_ISR_Restore(level);
		
		_Thread_queue_Enqueue(&mqueue->Wait_queue,timeout);
	}
	return LWP_MQ_STATUS_UNSATISFIED_WAIT;
}

u32 _CORE_message_queue_Broadcast(CORE_message_queue_Control *mqueue,void *buffer,u32 size,u32 id,u32 *count)
{
	Thread_Control *thread;
	u32 num_broadcast;
	Thread_Wait_information *waitp;
	u32 rsize;
#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Broadcast(%p,%p,%d,%d,%p)\n",mqueue,buffer,size,id,count);
#endif
	if(mqueue->number_of_pending_messages!=0) {
		*count = 0;
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	num_broadcast = 0;
	while((thread=_Thread_queue_Dequeue(&mqueue->Wait_queue))) {
		waitp = &thread->Wait;
		++num_broadcast;

		rsize = size;
		if(size>mqueue->maximum_message_size)
			rsize = mqueue->maximum_message_size;

		_CORE_message_queue_Copy_buffer(waitp->return_argument,buffer,rsize);
		*(u32*)waitp->return_argument = size;
	}
	*count = num_broadcast;
	return LWP_MQ_STATUS_SUCCESSFUL;
}

void _CORE_message_queue_Close(CORE_message_queue_Control *mqueue,u32 status)
{
	_Thread_queue_Flush(&mqueue->Wait_queue,status);
	_CORE_message_queue_Flush_support(mqueue);
	_Workspace_Free(mqueue->message_buffers);
}

u32 _CORE_message_queue_Flush(CORE_message_queue_Control *mqueue)
{
	if(mqueue->number_of_pending_messages!=0)
		return _CORE_message_queue_Flush_support(mqueue);
	else
		return 0;
}

u32 _CORE_message_queue_Flush_support(CORE_message_queue_Control *mqueue)
{
	u32 level;
	Chain_Node *inactive;
	Chain_Node *mqueue_first;
	Chain_Node *mqueue_last;
	u32 cnt;

	_CPU_ISR_Disable(level);

	inactive = mqueue->Inactive_messages.first;
	mqueue_first = mqueue->Pending_messages.first;
	mqueue_last = mqueue->Pending_messages.last;

	mqueue->Inactive_messages.first = mqueue_first;
	mqueue_last->next = inactive;
	inactive->previous = mqueue_last;
	mqueue_first->previous = _Chain_Head(&mqueue->Inactive_messages);
	
	_Chain_Initialize_empty(&mqueue->Pending_messages);

	cnt = mqueue->number_of_pending_messages;
	mqueue->number_of_pending_messages = 0;

	_CPU_ISR_Restore(level);
	return cnt;
}

void _CORE_message_queue_Flush_waiting_threads(CORE_message_queue_Control *mqueue)
{
	_Thread_queue_Flush(&mqueue->Wait_queue,LWP_MQ_STATUS_UNSATISFIED_NOWAIT);
}
