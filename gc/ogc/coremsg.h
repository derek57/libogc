/*  coremsg.h
 *
 *  This include file contains all the constants and structures associated
 *  with the Message queue Handler.
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 */

#ifndef __RTEMS_CORE_MESSAGE_QUEUE_h
#define __RTEMS_CORE_MESSAGE_QUEUE_h

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <limits.h>
#include <string.h>
#include <threadq.h>

//#define _LWPMQ_DEBUG

/*
 *  Blocking disciplines for a message_queue.
 */

typedef enum {
  /** This value indicates that blocking tasks are in FIFO order. */
  CORE_MESSAGE_QUEUE_DISCIPLINES_FIFO,
  /** This value indicates that blocking tasks are in priority order. */
  CORE_MESSAGE_QUEUE_DISCIPLINES_PRIORITY
}   CORE_message_queue_Disciplines;

/*
 *  Core Message queue handler return statuses.
 */

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

/*
 *  The following enumerated type details the modes in which a message
 *  may be submitted to a message queue.  The message may be posted
 *  in a send or urgent fashion.
 *
 *  NOTE:  All other values are message priorities.  Numerically smaller
 *         priorities indicate higher priority messages.
 *
 */

#define  CORE_MESSAGE_QUEUE_SEND_REQUEST   INT_MAX
#define  CORE_MESSAGE_QUEUE_URGENT_REQUEST INT_MIN

/*
 *  The following defines the type for a Notification handler.  A notification
 *  handler is invoked when the message queue makes a 0->1 transition on
 *  pending messages.
 */

typedef void (*CORE_message_queue_Notify_Handler)(void *);

/*
 *  The following defines the data types needed to manipulate
 *  the contents of message buffers.
 *
 *  NOTE:  The buffer field is normally longer than a single unsigned32.
 *         but since messages are variable length we just make a ptr to 1.  
 */

typedef struct {
	unsigned32 size;
	unsigned32 buffer[1];
} CORE_message_queue_Buffer;

/*
 *  The following records define the organization of a message
 *  buffer.
 */

typedef struct {
	Chain_Node Node;
	Priority_Control priority;
	CORE_message_queue_Buffer Contents;
} CORE_message_queue_Buffer_control;

/*
 *  The following defines the control block used to manage the 
 *  attributes of each message queue.
 */

typedef struct {
	unsigned32 discipline;
} CORE_message_queue_Attributes;

/*
 *  The following defines the control block used to manage each 
 *  counting message_queue.
 */

typedef struct {
	Thread_queue_Control Wait_queue;
	CORE_message_queue_Attributes Attributes;
	unsigned32 maximum_pending_messages;
	unsigned32 number_of_pending_messages;
	unsigned32 maximum_message_size;
	Chain_Control Pending_messages;
	CORE_message_queue_Buffer *message_buffers;
	CORE_message_queue_Notify_Handler notify_handler;
	void *notify_argument;
	Chain_Control Inactive_messages;
} CORE_message_queue_Control;

/*
 *  _CORE_message_queue_Initialize
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the message_queue based on the parameters passed.
 */

boolean _CORE_message_queue_Initialize(
  CORE_message_queue_Control    *the_message_queue,
  CORE_message_queue_Attributes *the_message_queue_attributes,
  unsigned32                     maximum_pending_messages,
  unsigned32                     maximum_message_size
);

/*
 *  _CORE_message_queue_Close
 *
 *  DESCRIPTION:
 *
 *  This function closes a message by returning all allocated space and
 *  flushing the message_queue's task wait queue.
 */
 
void _CORE_message_queue_Close(
  CORE_message_queue_Control *the_message_queue,
  unsigned32                  status
);

/*
 *  _CORE_message_queue_Seize
 *
 *  DESCRIPTION:
 *
 *  This kernel routine dequeues a message, copies the message buffer to
 *  a given destination buffer, and frees the message buffer to the
 *  inactive message pool.  The thread will be blocked if wait is TRUE,
 *  otherwise an error will be given to the thread if no messages are available.
 *
 *  NOTE: Returns message priority via return are in TCB.
 */
 
CORE_message_queue_Status _CORE_message_queue_Seize(
  CORE_message_queue_Control      *the_message_queue,
  Objects_Id                       id,
  void                            *buffer,
  unsigned32                      *size,
  boolean                          wait,
  Watchdog_Interval                timeout
);

/*
 *  _CORE_message_queue_Submit
 *
 *  DESCRIPTION:
 *
 *  This routine implements the send and urgent message functions. It
 *  processes a message that is to be submitted to the designated
 *  message queue.  The message will either be processed as a
 *  send message which it will be inserted at the rear of the queue
 *  or it will be processed as an urgent message which will be inserted
 *  at the front of the queue.
 *
 */
 
CORE_message_queue_Status _CORE_message_queue_Submit(
  CORE_message_queue_Control                *the_message_queue,
  Objects_Id                                 id,
  void                                      *buffer,
  unsigned32                                 size,
  CORE_message_queue_Submit_types            submit_type,
  boolean                                    wait,
  Watchdog_Interval                          timeout
);

/*
 *  _CORE_message_queue_Broadcast
 *
 *  DESCRIPTION:
 *
 *  This function sends a message for every thread waiting on the queue and
 *  returns the number of threads made ready by the message.
 *
 */
 
CORE_message_queue_Status _CORE_message_queue_Broadcast(
  CORE_message_queue_Control                *the_message_queue,
  void                                      *buffer,
  unsigned32                                 size,
  Objects_Id                                 id,
  unsigned32                                *count
);

/*
 *  _CORE_message_queue_Insert_message
 *
 *  DESCRIPTION:
 *
 *  This kernel routine inserts the specified message into the 
 *  message queue.  It is assumed that the message has been filled
 *  in before this routine is called.
 */

void _CORE_message_queue_Insert_message(
  CORE_message_queue_Control        *the_message_queue,
  CORE_message_queue_Buffer_control *the_message,
  CORE_message_queue_Submit_types    submit_type
);

/*
 *  _CORE_message_queue_Flush
 *
 *  DESCRIPTION:
 *
 *  This function flushes the message_queue's pending message queue.  The
 *  number of messages flushed from the queue is returned.
 *
 */

unsigned32 _CORE_message_queue_Flush(
  CORE_message_queue_Control *the_message_queue
);

/*
 *  _CORE_message_queue_Flush_support
 *
 *  DESCRIPTION:
 *
 *  This routine flushes all outstanding messages and returns
 *  them to the inactive message chain.
 */
 
unsigned32 _CORE_message_queue_Flush_support(
  CORE_message_queue_Control *the_message_queue
);
 
/*
 *  _CORE_message_queue_Flush_waiting_threads
 *
 *  DESCRIPTION:
 *
 *  This function flushes the threads which are blocked on this
 *  message_queue's pending message queue.  They are unblocked whether
 *  blocked sending or receiving.
 */

void _CORE_message_queue_Flush_waiting_threads(
  CORE_message_queue_Control *the_message_queue
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/coremsg.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
