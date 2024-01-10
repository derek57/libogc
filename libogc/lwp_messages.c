#include <stdlib.h>
#include "asm.h"
#include "lwp_messages.h"
#include "lwp_wkspace.h"

void _CORE_message_queue_Insert_message(CORE_message_queue_Control *mqueue,CORE_message_queue_Buffer_control *msg,u32 type)
{
	++mqueue->num_pendingmsgs;
	msg->prio = type;

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

			header = &mqueue->pending_msgs;
			node = header->first;
			while(!_Chain_Is_tail(header,node)) {
				tmsg = (CORE_message_queue_Buffer_control*)node;
				if(tmsg->prio<=msg->prio) {
					node = node->next;
					continue;
				}
				break;
			}
			_Chain_Insert(node->prev,&msg->node);
		}
		break;
	}
	
	if(mqueue->num_pendingmsgs==1 && mqueue->notify_handler)
		mqueue->notify_handler(mqueue->notify_arg);
}

u32 _CORE_message_queue_Initialize(CORE_message_queue_Control *mqueue,CORE_message_queue_Attributes *attrs,u32 max_pendingmsgs,u32 max_msgsize)
{
	u32 alloc_msgsize;
	u32 buffering_req;
	
#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Initialize(%p,%p,%d,%d)\n",mqueue,attrs,max_pendingmsgs,max_msgsize);
#endif
	mqueue->max_pendingmsgs = max_pendingmsgs;
	mqueue->num_pendingmsgs = 0;
	mqueue->max_msgsize = max_msgsize;
	_CORE_message_queue_Set_notify(mqueue,NULL,NULL);
	
	alloc_msgsize = max_msgsize;
	if(alloc_msgsize&(sizeof(u32)-1))
		alloc_msgsize = (alloc_msgsize+sizeof(u32))&~(sizeof(u32)-1);
	
	buffering_req = max_pendingmsgs*(alloc_msgsize+sizeof(CORE_message_queue_Buffer_control));
	mqueue->msq_buffers = (CORE_message_queue_Buffer*)_Workspace_Allocate(buffering_req);

	if(!mqueue->msq_buffers) return 0;

	_Chain_Initialize(&mqueue->inactive_msgs,mqueue->msq_buffers,max_pendingmsgs,(alloc_msgsize+sizeof(CORE_message_queue_Buffer_control)));
	_Chain_Initialize_empty(&mqueue->pending_msgs);
	_Thread_queue_Initialize(&mqueue->wait_queue,_CORE_message_queue_Is_priority(attrs)?LWP_THREADQ_MODEPRIORITY:LWP_THREADQ_MODEFIFO,LWP_STATES_WAITING_FOR_MESSAGE,LWP_MQ_STATUS_TIMEOUT);

	return 1;
}

u32 _CORE_message_queue_Seize(CORE_message_queue_Control *mqueue,u32 id,void *buffer,u32 *size,u32 wait,u64 timeout)
{
	u32 level;
	CORE_message_queue_Buffer_control *msg;
	Thread_Control *exec,*thread;

	exec = _thr_executing;
	exec->wait.return_code = LWP_MQ_STATUS_SUCCESSFUL;
#ifdef _LWPMQ_DEBUG
	printf("_CORE_message_queue_Seize(%p,%d,%p,%p,%d,%d)\n",mqueue,id,buffer,size,wait,mqueue->num_pendingmsgs);
#endif
	
	_CPU_ISR_Disable(level);
	if(mqueue->num_pendingmsgs!=0) {
		--mqueue->num_pendingmsgs;
		msg = _CORE_message_queue_Get_pending_message(mqueue);
		_CPU_ISR_Restore(level);
		
		*size = msg->contents.size;
		exec->wait.count = msg->prio;
		_CORE_message_queue_Copy_buffer(buffer,msg->contents.buffer,*size);

		thread = _Thread_queue_Dequeue(&mqueue->wait_queue);
		if(!thread) {
			_CORE_message_queue_Free_message_buffer(mqueue,msg);
			return LWP_MQ_STATUS_SUCCESSFUL;
		}

		msg->prio = thread->wait.count;
		msg->contents.size = (u32)thread->wait.return_argument_1;
		_CORE_message_queue_Copy_buffer(msg->contents.buffer,thread->wait.return_argument,msg->contents.size);
		
		_CORE_message_queue_Insert_message(mqueue,msg,msg->prio);
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	if(!wait) {
		_CPU_ISR_Restore(level);
		exec->wait.return_code = LWP_MQ_STATUS_UNSATISFIED_NOWAIT;
		return LWP_MQ_STATUS_UNSATISFIED_NOWAIT;
	}

	_Thread_queue_Enter_critical_section(&mqueue->wait_queue);
	exec->wait.queue = &mqueue->wait_queue;
	exec->wait.id = id;
	exec->wait.return_argument = (void*)buffer;
	exec->wait.return_argument_1 = (void*)size;
	_CPU_ISR_Restore(level);

	_Thread_queue_Enqueue(&mqueue->wait_queue,timeout);
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
	if(size>mqueue->max_msgsize)
		return LWP_MQ_STATUS_INVALID_SIZE;

	if(mqueue->num_pendingmsgs==0) {
		thread = _Thread_queue_Dequeue(&mqueue->wait_queue);
		if(thread) {
			_CORE_message_queue_Copy_buffer(thread->wait.return_argument,buffer,size);
			*(u32*)thread->wait.return_argument_1 = size;
			thread->wait.count = type;
			return LWP_MQ_STATUS_SUCCESSFUL;
		}
	}

	if(mqueue->num_pendingmsgs<mqueue->max_pendingmsgs) {
		msg = _CORE_message_queue_Allocate_message_buffer(mqueue);
		if(!msg) return LWP_MQ_STATUS_UNSATISFIED;

		_CORE_message_queue_Copy_buffer(msg->contents.buffer,buffer,size);
		msg->contents.size = size;
		msg->prio = type;
		_CORE_message_queue_Insert_message(mqueue,msg,type);
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	if(!wait) return LWP_MQ_STATUS_TOO_MANY;
	if(_ISR_Is_in_progress()) return LWP_MQ_STATUS_UNSATISFIED;

	{
		Thread_Control *exec = _thr_executing;

		_CPU_ISR_Disable(level);
		_Thread_queue_Enter_critical_section(&mqueue->wait_queue);
		exec->wait.queue = &mqueue->wait_queue;
		exec->wait.id = id;
		exec->wait.return_argument = (void*)buffer;
		exec->wait.return_argument_1 = (void*)size;
		exec->wait.count = type;
		_CPU_ISR_Restore(level);
		
		_Thread_queue_Enqueue(&mqueue->wait_queue,timeout);
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
	if(mqueue->num_pendingmsgs!=0) {
		*count = 0;
		return LWP_MQ_STATUS_SUCCESSFUL;
	}

	num_broadcast = 0;
	while((thread=_Thread_queue_Dequeue(&mqueue->wait_queue))) {
		waitp = &thread->wait;
		++num_broadcast;

		rsize = size;
		if(size>mqueue->max_msgsize)
			rsize = mqueue->max_msgsize;

		_CORE_message_queue_Copy_buffer(waitp->return_argument,buffer,rsize);
		*(u32*)waitp->return_argument = size;
	}
	*count = num_broadcast;
	return LWP_MQ_STATUS_SUCCESSFUL;
}

void _CORE_message_queue_Close(CORE_message_queue_Control *mqueue,u32 status)
{
	_Thread_queue_Flush(&mqueue->wait_queue,status);
	_CORE_message_queue_Flush_support(mqueue);
	_Workspace_Free(mqueue->msq_buffers);
}

u32 _CORE_message_queue_Flush(CORE_message_queue_Control *mqueue)
{
	if(mqueue->num_pendingmsgs!=0)
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

	inactive = mqueue->inactive_msgs.first;
	mqueue_first = mqueue->pending_msgs.first;
	mqueue_last = mqueue->pending_msgs.last;

	mqueue->inactive_msgs.first = mqueue_first;
	mqueue_last->next = inactive;
	inactive->prev = mqueue_last;
	mqueue_first->prev = _Chain_Head(&mqueue->inactive_msgs);
	
	_Chain_Initialize_empty(&mqueue->pending_msgs);

	cnt = mqueue->num_pendingmsgs;
	mqueue->num_pendingmsgs = 0;

	_CPU_ISR_Restore(level);
	return cnt;
}

void _CORE_message_queue_Flush_waiting_threads(CORE_message_queue_Control *mqueue)
{
	_Thread_queue_Flush(&mqueue->wait_queue,LWP_MQ_STATUS_UNSATISFIED_NOWAIT);
}
