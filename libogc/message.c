/*-------------------------------------------------------------

message.c -- Thread subsystem II

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


#include <asm.h>
#include <stdlib.h>
#include <lwp_messages.h>
#include <lwp_objmgr.h>
#include <lwp_config.h>
#include "message.h"

#define LWP_CHECK_MBOX( hndl )  \
{                               \
	if ( ( ( hndl ) == MQ_BOX_NULL) || ( _Objects_Get_node( hndl ) != OBJECTS_POSIX_MESSAGE_QUEUES ) )  \
		return NULL;            \
}

/*
 *  Data Structure used to manage a POSIX message queue
 */

typedef struct
{
	Objects_Control Object;
	CORE_message_queue_Control Message_queue;
} POSIX_Message_queue_Control;

/*
 *  The following defines the information control block used to manage
 *  this class of objects.  The second item is used to manage the set
 *  of "file descriptors" associated with the message queues.
 */

Objects_Information _POSIX_Message_queue_Information;

/*PAGE
 *
 *  _POSIX_Message_queue_Manager_initialization
 *
 *  This routine initializes all message_queue manager related data structures.
 *
 *  Input parameters:
 *    maximum_message_queues - maximum configured message_queues
 *
 *  Output parameters:  NONE
 */

void _POSIX_Message_queue_Manager_initialization( void )
{
  _Objects_Initialize_information(
    &_POSIX_Message_queue_Information,
    CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES,
    sizeof( POSIX_Message_queue_Control )
  );
}

/*
 *  _POSIX_Message_queue_Get
 *
 *  DESCRIPTION:
 *
 *  This function maps message queue IDs to message queue control blocks.
 *  If ID corresponds to a local message queue, then it returns
 *  the_mq control pointer which maps to ID and location
 *  is set to OBJECTS_LOCAL.  if the message queue ID is global and
 *  resides on a remote node, then location is set to OBJECTS_REMOTE,
 *  and the_message queue is undefined.  Otherwise, location is set
 *  to OBJECTS_ERROR and tµhe_mq is undefined.
 */

RTEMS_INLINE_ROUTINE POSIX_Message_queue_Control *_POSIX_Message_queue_Get(
  mqd_t mbox
)
{
  LWP_CHECK_MBOX( mbox );
  return (POSIX_Message_queue_Control *)_Objects_Get( &_POSIX_Message_queue_Information, _Objects_Get_index( mbox ) );
}

/*
 *  _POSIX_Message_queue_Free
 *
 *  DESCRIPTION:
 *
 *  This routine frees a message queue control block to the
 *  inactive chain of free message queue control blocks.
 */

RTEMS_INLINE_ROUTINE void _POSIX_Message_queue_Free(
  POSIX_Message_queue_Control *the_mq
)
{
  _Objects_Close( &_POSIX_Message_queue_Information, &the_mq->Object );
  _Objects_Free( &_POSIX_Message_queue_Information, &the_mq->Object );
}

/*
 *  _POSIX_Message_queue_Allocate
 *
 *  DESCRIPTION:
 *
 *  This function allocates a message queue control block from
 *  the inactive chain of free message queue control blocks.
 */

STATIC POSIX_Message_queue_Control *_POSIX_Message_queue_Allocate( void )
{
  POSIX_Message_queue_Control *the_mq;

  _Thread_Disable_dispatch();
  the_mq = (POSIX_Message_queue_Control *)_Objects_Allocate( &_POSIX_Message_queue_Information );

  if ( the_mq ) {
    _Objects_Open( &_POSIX_Message_queue_Information, &the_mq->Object );
    return the_mq;
  }

  _Thread_Enable_dispatch();
  return NULL;
}

/*PAGE
 *
 *  15.2.2 Open a Message Queue, P1003.1b-1993, p. 272
 */

int mq_open(
  mqd_t      *mqdes,
  unsigned32  count
)
{
  CORE_message_queue_Attributes  the_mq_attr;
  POSIX_Message_queue_Control   *the_mq = NULL;

  if ( !mqdes )
    rtems_set_errno_and_return_minus_one( EINVAL ); 

  the_mq = _POSIX_Message_queue_Allocate();

  if ( !the_mq )
    return MQ_ERROR_TOOMANY;

  /* XXX
   *
   *  Note that thread blocking discipline should be based on the
   *  current scheduling policy.
   */

  the_mq_attr.discipline = CORE_MESSAGE_QUEUE_DISCIPLINES_FIFO;

  /*
   * errno was set by Create_support, so don't set it again.
   */

  if ( !_CORE_message_queue_Initialize(
          &the_mq->Message_queue,
          &the_mq_attr,
          count,
          sizeof( char * )
      ) ) {
    _POSIX_Message_queue_Free( the_mq );
    _Thread_Enable_dispatch();
    return MQ_ERROR_TOOMANY;
  }

  *mqdes = (mqd_t)_Objects_Build_id( OBJECTS_POSIX_MESSAGE_QUEUES, _Objects_Get_index( the_mq->Object.id ) );
  _Thread_Enable_dispatch();
  return MQ_ERROR_SUCCESSFUL;
}

/*
 *
 *  15.2.2 Close a Message Queue, P1003.1b-1993, p. 275
 */

void mq_close(
  mqd_t  mqdes
)
{
  POSIX_Message_queue_Control *the_mq;

  the_mq = _POSIX_Message_queue_Get( mqdes );

  if ( !the_mq )
    return;

  _CORE_message_queue_Close( &the_mq->Message_queue, 0 );
  _Thread_Enable_dispatch();

  _POSIX_Message_queue_Free( the_mq );
}

/*PAGE
 *
 *  15.2.4 Send a Message to a Message Queue, P1003.1b-1993, p. 277
 *
 *  NOTE: P1003.4b/D8, p. 45 adds mq_timedsend().
 */

boolean mq_send(
  mqd_t      mqdes,
  char      *msg_ptr,
  unsigned32 oflag
)
{
  boolean                      msg_status;
  POSIX_Message_queue_Control *the_mq;
  unsigned32                   wait = (oflag == MQ_MSG_BLOCK ) ? TRUE : FALSE;

  the_mq = _POSIX_Message_queue_Get( mqdes );

  if ( !the_mq )
    return FALSE;

  msg_status = FALSE;

  if ( _CORE_message_queue_Submit(
      &the_mq->Message_queue,
      the_mq->Object.id,
      (void *)&msg_ptr,
      sizeof( char * ),
      CORE_MESSAGE_QUEUE_SEND_REQUEST,
      wait,
      RTEMS_NO_TIMEOUT ) == CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL
    )
    msg_status = TRUE;

  _Thread_Enable_dispatch();

  return msg_status;
}

/*PAGE
 *
 *  15.2.5 Receive a Message From a Message Queue, P1003.1b-1993, p. 279
 *
 *  NOTE: P1003.4b/D8, p. 45 adds mq_timedreceive().
 */

boolean mq_receive(
  mqd_t         mqdes,
  char         *msg_ptr,
  unsigned32    oflag
)
{
  boolean                      msg_status;
  POSIX_Message_queue_Control *the_mq;
  unsigned32                   tmp;
  unsigned32                   wait = (oflag == MQ_MSG_BLOCK) ? TRUE : FALSE;

  the_mq = _POSIX_Message_queue_Get( mqdes );

  if ( !the_mq )
    return FALSE;

  msg_status = FALSE;

  if ( _CORE_message_queue_Seize(
      &the_mq->Message_queue,
      the_mq->Object.id,
      (void *)msg_ptr,
      &tmp,
      wait,
      RTEMS_NO_TIMEOUT) == CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL
	)
    msg_status = TRUE;

  _Thread_Enable_dispatch();

  return msg_status;
}

boolean MQ_Jam(
  mqd_t       mqdes,
  char       *msg_ptr,
  unsigned32  oflag
)
{
  boolean                      msg_status;
  POSIX_Message_queue_Control *the_mq;
  unsigned32                   wait = (oflag == MQ_MSG_BLOCK) ? TRUE : FALSE;

  the_mq = _POSIX_Message_queue_Get( mqdes );

  if ( !the_mq )
    return FALSE;

  msg_status = FALSE;

  if ( _CORE_message_queue_Submit(
      &the_mq->Message_queue,
      the_mq->Object.id,
      (void *)&msg_ptr,
      sizeof( char * ),
      CORE_MESSAGE_QUEUE_URGENT_REQUEST,
      wait,
      RTEMS_NO_TIMEOUT) == CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL
    )
    msg_status = TRUE;

  _Thread_Enable_dispatch();

  return msg_status;
}
