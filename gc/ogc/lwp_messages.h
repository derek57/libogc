#ifndef __LWP_MESSAGES_H__
#define __LWP_MESSAGES_H__

#include <gctypes.h>
#include <limits.h>
#include <string.h>
#include <lwp_threadq.h>

//#define _LWPMQ_DEBUG

#define LWP_MQ_FIFO							0
#define LWP_MQ_PRIORITY						1

#define LWP_MQ_STATUS_SUCCESSFUL			0
#define LWP_MQ_STATUS_INVALID_SIZE			1
#define LWP_MQ_STATUS_TOO_MANY				2
#define LWP_MQ_STATUS_UNSATISFIED			3
#define LWP_MQ_STATUS_UNSATISFIED_NOWAIT	4
#define LWP_MQ_STATUS_DELETED				5
#define LWP_MQ_STATUS_TIMEOUT				6
#define LWP_MQ_STATUS_UNSATISFIED_WAIT		7

#define LWP_MQ_SEND_REQUEST					INT_MAX
#define LWP_MQ_SEND_URGENT					INT_MIN

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CORE_message_queue_Notify_Handler)(void *);

typedef struct {
	u32 size;
	u32 buffer[1];
} CORE_message_queue_Buffer;

typedef struct {
	Chain_Node node;
	u32 prio;
	CORE_message_queue_Buffer contents;
} CORE_message_queue_Buffer_control;

//the following struct is extensible
typedef struct {
	u32 mode;
} CORE_message_queue_Attributes;

typedef struct {
	Thread_queue_Control wait_queue;
	CORE_message_queue_Attributes attr;
	u32 max_pendingmsgs;
	u32 num_pendingmsgs;
	u32 max_msgsize;
	Chain_Control pending_msgs;
	CORE_message_queue_Buffer *msq_buffers;
	CORE_message_queue_Notify_Handler notify_handler;
	void *notify_arg;
	Chain_Control inactive_msgs;
} CORE_message_queue_Control;

u32 _CORE_message_queue_Initialize(CORE_message_queue_Control *mqueue,CORE_message_queue_Attributes *attrs,u32 max_pendingmsgs,u32 max_msgsize);
void _CORE_message_queue_Close(CORE_message_queue_Control *mqueue,u32 status);
u32 _CORE_message_queue_Seize(CORE_message_queue_Control *mqueue,u32 id,void *buffer,u32 *size,u32 wait,u64 timeout);
u32 _CORE_message_queue_Submit(CORE_message_queue_Control *mqueue,u32 id,void *buffer,u32 size,u32 type,u32 wait,u64 timeout);
u32 _CORE_message_queue_Broadcast(CORE_message_queue_Control *mqueue,void *buffer,u32 size,u32 id,u32 *count);
void _CORE_message_queue_Insert_message(CORE_message_queue_Control *mqueue,CORE_message_queue_Buffer_control *msg,u32 type);
u32 _CORE_message_queue_Flush(CORE_message_queue_Control *mqueue);
u32 _CORE_message_queue_Flush_support(CORE_message_queue_Control *mqueue);
void _CORE_message_queue_Flush_waiting_threads(CORE_message_queue_Control *mqueue);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_messages.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
