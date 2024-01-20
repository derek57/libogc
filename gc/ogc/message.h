/*  rtems/posix/mqueue.h
 *
 *  This include file contains all the private support information for
 *  POSIX Message Queues.
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


#ifndef __MESSAGE_H__
#define __MESSAGE_H__

/*! \file message.h 
\brief Thread subsystem II

*/ 

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>

#define MQ_BOX_NULL				0xffffffff

#define MQ_ERROR_SUCCESSFUL		0
#define MQ_ERROR_TOOMANY		-5

#define MQ_MSG_BLOCK			0
#define MQ_MSG_NOBLOCK			1


/*! \typedef unsigned32 mqd_t
\brief typedef for the message queue handle
*/
typedef Objects_Id  mqd_t;


/*! \fn unsigned32 mq_open(mqd_t *mqbox,unsigned32 count)
\brief Initializes a message queue
\param[out] mqbox pointer to the mqd_t handle.
\param[in] count maximum number of messages the queue can hold

\return 0 on success, <0 on error
*/
int mq_open(
  mqd_t      *mqdes,
  unsigned32  count
);


/*! \fn void mq_close(mqd_t mqbox)
\brief Closes the message queue and releases all memory.
\param[in] mqbox handle to the mqd_t structure.

\return none
*/
void mq_close(
  mqd_t  mqdes
);


/*! \fn boolean mq_send(mqd_t mqbox,char *msg,unsigned32 flags)
\brief Sends a message to the given message queue.
\param[in] mqbox mqd_t handle to the message queue
\param[in] msg message to send
\param[in] flags message flags (MQ_MSG_BLOCK, MQ_MSG_NOBLOCK)

\return boolean result
*/
boolean mq_send(
  mqd_t      mqdes,
  char      *msg_ptr,
  unsigned32 oflag
);


/*! \fn boolean MQ_Jam(mqd_t mqdes, char *msg_ptr, unsigned32 oflag)
\brief Sends a message to the given message queue and jams it in front of the queue.
\param[in] mqdes mqd_t handle to the message queue
\param[in] msg_ptr message to send
\param[in] oflag message flags (MQ_MSG_BLOCK, MQ_MSG_NOBLOCK)

\return boolean result
*/
boolean MQ_Jam(mqd_t mqdes, char *msg_ptr, unsigned32 oflag);


/*! \fn boolean mq_receive(mqd_t mqbox,char **msg,unsigned32 oflag)
\brief Sends a message to the given message queue.
\param[in] mqdes mqd_t handle to the message queue
\param[in] msg_ptr pointer to a mqmsg_t_t-type message to receive.
\param[in] oflag message flags (MQ_MSG_BLOCK, MQ_MSG_NOBLOCK)

\return boolean result
*/
boolean mq_receive(
  mqd_t         mqdes,
  char         *msg_ptr,
  unsigned32    oflag
);

#ifdef __cplusplus
	}
#endif

#endif
