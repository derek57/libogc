/*-------------------------------------------------------------

message.h -- Thread subsystem II

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/


#ifndef __MESSAGE_H__
#define __MESSAGE_H__

/*! \file message.h 
\brief Thread subsystem II

*/ 

#include <gctypes.h>

#define MQ_BOX_NULL				0xffffffff

#define MQ_ERROR_SUCCESSFUL		0
#define MQ_ERROR_TOOMANY		-5

#define MQ_MSG_BLOCK			0
#define MQ_MSG_NOBLOCK			1


#ifdef __cplusplus
extern "C" {
#endif


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
