#ifndef __LWP_MESSAGES_H__
#define __LWP_MESSAGES_H__

#include <gctypes.h>
#include <limits.h>
#include <string.h>
#include <lwp_threadq.h>

//#define _LWPMQ_DEBUG

typedef enum {
  /** This value indicates that blocking tasks are in FIFO order. */
  CORE_MESSAGE_QUEUE_DISCIPLINES_FIFO,
  /** This value indicates that blocking tasks are in priority order. */
  CORE_MESSAGE_QUEUE_DISCIPLINES_PRIORITY
}   CORE_message_queue_Disciplines;

typedef enum {
  /** This value indicates the operation completed sucessfully. */
  CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL,
  /** This value indicates that the message was too large for this queue. */
  CORE_MESSAGE_QUEUE_STATUS_INVALID_SIZE,
  /** This value indicates that there are too many messages pending. */
  CORE_MESSAGE_QUEUE_STATUS_TOO_MANY,
  /** This value indicates that a receive was unsuccessful. */
  CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED,
  /** This value indicates that a blocking send was unsuccessful. */
  CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED_NOWAIT,
  /** This value indicates that the message queue being blocked upon
   *  was deleted while the thread was waiting.
   */
  CORE_MESSAGE_QUEUE_STATUS_WAS_DELETED,
  /** This value indicates that the thread had to timeout while waiting
   *  to receive a message because one did not become available.
   */
  CORE_MESSAGE_QUEUE_STATUS_TIMEOUT,
  /** This value indicates that a blocking receive was unsuccessful. */
  CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED_WAIT
}   CORE_message_queue_Status;

#define CORE_MESSAGE_QUEUE_SEND_REQUEST					INT_MAX
#define CORE_MESSAGE_QUEUE_URGENT_REQUEST					INT_MIN

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CORE_message_queue_Notify_Handler)(void *);

typedef struct {
	u32 size;
	u32 buffer[1];
} CORE_message_queue_Buffer;

typedef struct {
	Chain_Node Node;
	u32 priority;
	CORE_message_queue_Buffer Contents;
} CORE_message_queue_Buffer_control;

//the following struct is extensible
typedef struct {
	u32 discipline;
} CORE_message_queue_Attributes;

typedef struct {
	Thread_queue_Control Wait_queue;
	CORE_message_queue_Attributes Attributes;
	u32 maximum_pending_messages;
	u32 number_of_pending_messages;
	u32 maximum_message_size;
	Chain_Control Pending_messages;
	CORE_message_queue_Buffer *message_buffers;
	CORE_message_queue_Notify_Handler notify_handler;
	void *notify_argument;
	Chain_Control Inactive_messages;
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
