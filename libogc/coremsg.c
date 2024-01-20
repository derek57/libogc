/*
 *  CORE Message Queue Handler
 *
 *  DESCRIPTION:
 *
 *  This package is the implementation of the CORE Message Queue Handler.
 *  This core object provides task synchronization and communication functions
 *  via messages passed to queue objects.
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

#include <stdlib.h>
#include "asm.h"
#include "coremsg.h"
#include "wkspace.h"

/*PAGE
 *
 *  _CORE_message_queue_Insert_message
 *
 *  This kernel routine inserts the specified message into the
 *  message queue.  It is assumed that the message has been filled
 *  in before this routine is called.
 *
 *  Input parameters:
 *    the_message_queue - pointer to message queue
 *    the_message       - message to insert
 *    priority          - insert indication
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    insert
 */

void _CORE_message_queue_Insert_message(
  CORE_message_queue_Control        *the_message_queue,
  CORE_message_queue_Buffer_control *the_message,
  CORE_message_queue_Submit_types    submit_type
)
{
  the_message_queue->number_of_pending_messages += 1;

  the_message->priority = submit_type;

  switch ( submit_type ) {
    case CORE_MESSAGE_QUEUE_SEND_REQUEST:
      _CORE_message_queue_Append( the_message_queue, the_message );
      break;
    case CORE_MESSAGE_QUEUE_URGENT_REQUEST:
      _CORE_message_queue_Prepend( the_message_queue, the_message );
      break;
    default:
      /* XXX interrupt critical section needs to be addressed */
      {
        CORE_message_queue_Buffer_control *this_message;
        Chain_Node                        *the_node;
        Chain_Control                     *the_header;

        the_header = &the_message_queue->Pending_messages;
        the_node = the_header->first;
        while ( !_Chain_Is_tail( the_header, the_node ) ) {

          this_message = (CORE_message_queue_Buffer_control *) the_node;

          if ( this_message->priority <= the_message->priority ) {
            the_node = the_node->next;
            continue;
          }

          break;
        }
        _Chain_Insert( the_node->previous, &the_message->Node );
      }
      break;
  }

  /*
   *  According to POSIX, does this happen before or after the message
   *  is actually enqueued.  It is logical to think afterwards, because
   *  the message is actually in the queue at this point.
   */

  if ( the_message_queue->number_of_pending_messages == 1 && 
       the_message_queue->notify_handler )
    (*the_message_queue->notify_handler)( the_message_queue->notify_argument );
}

/*PAGE
 *
 *  _CORE_message_queue_Initialize
 *
 *  This routine initializes a newly created message queue based on the
 *  specified data.
 *
 *  Input parameters:
 *    the_message_queue            - the message queue to initialize
 *    the_message_queue_attributes - the message queue's attributes
 *    maximum_pending_messages     - maximum message and reserved buffer count
 *    maximum_message_size         - maximum size of each message
 *
 *  Output parameters:
 *    TRUE   - if the message queue is initialized
 *    FALSE  - if the message queue is NOT initialized
 */

boolean _CORE_message_queue_Initialize(
  CORE_message_queue_Control    *the_message_queue,
  CORE_message_queue_Attributes *the_message_queue_attributes,
  unsigned32                     maximum_pending_messages,
  unsigned32                     maximum_message_size
)
{
  unsigned32 message_buffering_required;
  unsigned32 allocated_message_size;

  the_message_queue->maximum_pending_messages   = maximum_pending_messages;
  the_message_queue->number_of_pending_messages = 0;
  the_message_queue->maximum_message_size       = maximum_message_size;
  _CORE_message_queue_Set_notify( the_message_queue, NULL, NULL );
 
  /*
   * round size up to multiple of a ptr for chain init
   */
 
  allocated_message_size = maximum_message_size;
  if (allocated_message_size & (sizeof(unsigned32) - 1)) {
    allocated_message_size = ( allocated_message_size + sizeof(unsigned32) ) & ~(sizeof(unsigned32) - 1);
  }
   
  message_buffering_required = maximum_pending_messages *
       (allocated_message_size + sizeof(CORE_message_queue_Buffer_control));
 
  the_message_queue->message_buffers = (CORE_message_queue_Buffer *) 
     _Workspace_Allocate( message_buffering_required );
 
  if (the_message_queue->message_buffers == 0)
    return FALSE;
 
  _Chain_Initialize (
    &the_message_queue->Inactive_messages,
    the_message_queue->message_buffers,
    maximum_pending_messages,
    allocated_message_size + sizeof( CORE_message_queue_Buffer_control )
  );
 
  _Chain_Initialize_empty( &the_message_queue->Pending_messages );
 
  _Thread_queue_Initialize(
    &the_message_queue->Wait_queue,
    _CORE_message_queue_Is_priority( the_message_queue_attributes ) ?
       THREAD_QUEUE_DISCIPLINE_PRIORITY : THREAD_QUEUE_DISCIPLINE_FIFO,
    STATES_WAITING_FOR_MESSAGE,
    CORE_MESSAGE_QUEUE_STATUS_TIMEOUT
  );

  return TRUE;
} 

/*PAGE
 *
 *  _CORE_message_queue_Seize
 *
 *  This kernel routine dequeues a message, copies the message buffer to
 *  a given destination buffer, and frees the message buffer to the
 *  inactive message pool.  The thread will be blocked if wait is TRUE,
 *  otherwise an error will be given to the thread if no messages are available.
 *
 *  Input parameters:
 *    the_message_queue - pointer to message queue
 *    id                - id of object we are waitig on
 *    buffer            - pointer to message buffer to be filled
 *    size              - pointer to the size of buffer to be filled
 *    wait              - TRUE if wait is allowed, FALSE otherwise
 *    timeout           - time to wait for a message
 *
 *  Output parameters:
 *    CORE_MESSAGE_QUEUE_SUCCESSFUL - if successful
 *    error code                    - if unsuccessful
 *
 *  NOTE: Dependent on BUFFER_LENGTH
 *
 *  INTERRUPT LATENCY:
 *    available
 *    wait
 */

CORE_message_queue_Status _CORE_message_queue_Seize(
  CORE_message_queue_Control      *the_message_queue,
  Objects_Id                       id,
  void                            *buffer,
  unsigned32                      *size,
  boolean                          wait,
  Watchdog_Interval                timeout
)
{
  ISR_Level                          level;
  CORE_message_queue_Buffer_control *the_message;
  Thread_Control                    *executing;
  Thread_Control                    *the_thread;

  executing = _Thread_Executing;
  executing->Wait.return_code = CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL;
  _ISR_Disable( level );
  if ( the_message_queue->number_of_pending_messages != 0 ) {
    the_message_queue->number_of_pending_messages -= 1;

    the_message = _CORE_message_queue_Get_pending_message( the_message_queue );
    _ISR_Enable( level );

    *size = the_message->Contents.size;
    executing->Wait.count = the_message->priority;
    _CORE_message_queue_Copy_buffer(buffer,the_message->Contents.buffer,*size);

    /*
     *  There could be a thread waiting to send a message.  If there
     *  is not, then we can go ahead and free the buffer.
     *
     *  NOTE: If we note that the queue was not full before this receive,
     *  then we can avoid this dequeue.
     */

    the_thread = _Thread_queue_Dequeue( &the_message_queue->Wait_queue );
    if ( !the_thread ) {
      _CORE_message_queue_Free_message_buffer( the_message_queue, the_message );
      return CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL;
    }

    /*
     *  There was a thread waiting to send a message.  This code 
     *  puts the messages in the message queue on behalf of the 
     *  waiting task.
     */

    the_message->priority  = the_thread->Wait.count;
    the_message->Contents.size = (unsigned32)the_thread->Wait.return_argument_1;
    _CORE_message_queue_Copy_buffer(
      the_message->Contents.buffer,
      the_thread->Wait.return_argument,
      the_message->Contents.size
    );

    _CORE_message_queue_Insert_message(
       the_message_queue,
       the_message,
       the_message->priority
    );
    return CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL;
  }

  if ( !wait ) {
    _ISR_Enable( level );
    executing->Wait.return_code = CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED_NOWAIT;
    return CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED_NOWAIT;
  }

  _Thread_queue_Enter_critical_section( &the_message_queue->Wait_queue );
  executing->Wait.queue              = &the_message_queue->Wait_queue;
  executing->Wait.id                 = id;
  executing->Wait.return_argument    = (void *)buffer;
  executing->Wait.return_argument_1  = (void *)size;
  /* Wait.count will be filled in with the message priority */
  _ISR_Enable( level );

  _Thread_queue_Enqueue( &the_message_queue->Wait_queue, timeout );

  return CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL;
}

/*PAGE
 *
 *  _CORE_message_queue_Submit
 *
 *  This routine implements the send and urgent message functions. It
 *  processes a message that is to be submitted to the designated
 *  message queue.  The message will either be processed as a
 *  send message which it will be inserted at the rear of the queue
 *  or it will be processed as an urgent message which will be inserted
 *  at the front of the queue.
 *
 *  Input parameters:
 *    the_message_queue            - message is submitted to this message queue
 *    id                           - id of message queue
 *    buffer                       - pointer to message buffer
 *    size                         - size in bytes of message to send
 *    submit_type                  - send or urgent message
 *
 *  Output parameters:
 *    CORE_MESSAGE_QUEUE_SUCCESSFUL - if successful
 *    error code                    - if unsuccessful
 */

CORE_message_queue_Status _CORE_message_queue_Submit(
  CORE_message_queue_Control                *the_message_queue,
  Objects_Id                                 id,
  void                                      *buffer,
  unsigned32                                 size,
  CORE_message_queue_Submit_types            submit_type,
  boolean                                    wait,
  Watchdog_Interval                          timeout
)
{
  ISR_Level                            level;
  Thread_Control                      *the_thread;
  CORE_message_queue_Buffer_control   *the_message;

  if ( size > the_message_queue->maximum_message_size ) {
    return CORE_MESSAGE_QUEUE_STATUS_INVALID_SIZE;
  }

  /*
   *  Is there a thread currently waiting on this message queue?
   */
      
  if ( the_message_queue->number_of_pending_messages == 0 ) {
    the_thread = _Thread_queue_Dequeue( &the_message_queue->Wait_queue );
    if ( the_thread ) {
      _CORE_message_queue_Copy_buffer(
        the_thread->Wait.return_argument,
        buffer,
        size
      );
      *(unsigned32 *)the_thread->Wait.return_argument_1 = size;
      the_thread->Wait.count = submit_type;
    
#if defined(RTEMS_MULTIPROCESSING)
      if ( !_Objects_Is_local_id( the_thread->Object.id ) )
        (*api_message_queue_mp_support) ( the_thread, id );
#endif
      return CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL;
    }
  }

  /*
   *  No one waiting on the message queue at this time, so attempt to
   *  queue the message up for a future receive.
   */

  if ( the_message_queue->number_of_pending_messages <
       the_message_queue->maximum_pending_messages ) {

    the_message =
        _CORE_message_queue_Allocate_message_buffer( the_message_queue );

    /*
     *  NOTE: If the system is consistent, this error should never occur.
     */
    if ( !the_message ) {
      return CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED;
    }

    _CORE_message_queue_Copy_buffer(
      the_message->Contents.buffer,
      buffer,
      size
    );
    the_message->Contents.size = size;
    the_message->priority  = submit_type;

    _CORE_message_queue_Insert_message(
       the_message_queue,
       the_message,
       submit_type
    );
    return CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL;
  }

  /*
   *  No message buffers were available so we may need to return an
   *  overflow error or block the sender until the message is placed
   *  on the queue.
   */

  if ( !wait ) {
    return CORE_MESSAGE_QUEUE_STATUS_TOO_MANY;
  }

  if ( _ISR_Is_in_progress() )
    return CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED;

  {
    Thread_Control *executing = _Thread_Executing;

    _ISR_Disable( level );
    _Thread_queue_Enter_critical_section( &the_message_queue->Wait_queue );
    executing->Wait.queue              = &the_message_queue->Wait_queue;
    executing->Wait.id                 = id;
    executing->Wait.return_argument    = (void *)buffer;
    executing->Wait.return_argument_1  = (void *)size;
    executing->Wait.count              = submit_type;
    _ISR_Enable( level );

    _Thread_queue_Enqueue( &the_message_queue->Wait_queue, timeout );
  }

  return CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED_WAIT;
}

/*PAGE
 *
 *  _CORE_message_queue_Broadcast
 *
 *  This function sends a message for every thread waiting on the queue and
 *  returns the number of threads made ready by the message.
 *
 *  Input parameters:
 *    the_message_queue            - message is submitted to this message queue
 *    buffer                       - pointer to message buffer
 *    size                         - size in bytes of message to send
 *    id                           - id of message queue
 *    count                        - area to store number of threads made ready
 *
 *  Output parameters:
 *    count                         - number of threads made ready
 *    CORE_MESSAGE_QUEUE_SUCCESSFUL - if successful
 *    error code                    - if unsuccessful
 */

CORE_message_queue_Status _CORE_message_queue_Broadcast(
  CORE_message_queue_Control                *the_message_queue,
  void                                      *buffer,
  unsigned32                                 size,
  Objects_Id                                 id,
  unsigned32                                *count
)
{
  Thread_Control          *the_thread;
  unsigned32               number_broadcasted;
  Thread_Wait_information *waitp;
  unsigned32               constrained_size;

  /*
   *  If there are pending messages, then there can't be threads
   *  waiting for us to send them a message.
   *
   *  NOTE: This check is critical because threads can block on
   *        send and receive and this ensures that we are broadcasting
   *        the message to threads waiting to receive -- not to send.
   */

  if ( the_message_queue->number_of_pending_messages != 0 ) {
    *count = 0;
    return CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL;
  }

  /*
   *  There must be no pending messages if there is a thread waiting to
   *  receive a message.
   */

  number_broadcasted = 0;
  while ((the_thread = _Thread_queue_Dequeue(&the_message_queue->Wait_queue))) {
    waitp = &the_thread->Wait;
    number_broadcasted += 1;

    constrained_size = size;
    if ( size > the_message_queue->maximum_message_size )
        constrained_size = the_message_queue->maximum_message_size;

    _CORE_message_queue_Copy_buffer(
      waitp->return_argument,
      buffer,
      constrained_size
    );

    *(unsigned32 *)waitp->return_argument_1 = size;

#if defined(RTEMS_MULTIPROCESSING)
    if ( !_Objects_Is_local_id( the_thread->Object.id ) )
      (*api_message_queue_mp_support) ( the_thread, id );
#endif

  }
  *count = number_broadcasted;
  return CORE_MESSAGE_QUEUE_STATUS_SUCCESSFUL;
}

/*PAGE
 *
 *  _CORE_message_queue_Close
 *
 *  This function closes a message by returning all allocated space and
 *  flushing the message_queue's task wait queue.
 *
 *  Input parameters:
 *    the_message_queue      - the message_queue to be flushed
 *    status                 - status to pass to thread
 *
 *  Output parameters:  NONE
 */
 
void _CORE_message_queue_Close(
  CORE_message_queue_Control *the_message_queue,
  unsigned32                  status
)
{

  /*
   *  This will flush blocked threads whether they were blocked on
   *  a send or receive.
   */

  _Thread_queue_Flush(
    &the_message_queue->Wait_queue,
    status
  );

  /*
   *  This removes all messages from the pending message queue.  Since
   *  we just flushed all waiting threads, we don't have to worry about
   *  the flush satisfying any blocked senders as a side-effect.
   */
 
  (void) _CORE_message_queue_Flush_support( the_message_queue );

  (void) _Workspace_Free( the_message_queue->message_buffers );

}

/*PAGE
 *
 *  _CORE_message_queue_Flush
 *
 *  This function flushes the message_queue's pending message queue.  The
 *  number of messages flushed from the queue is returned.
 *
 *  Input parameters:
 *    the_message_queue - the message_queue to be flushed
 *
 *  Output parameters:
 *    returns - the number of messages flushed from the queue
 */
 
unsigned32 _CORE_message_queue_Flush(
  CORE_message_queue_Control *the_message_queue
)
{
  if ( the_message_queue->number_of_pending_messages != 0 )
    return _CORE_message_queue_Flush_support( the_message_queue );
  else
    return 0;
}

/*PAGE
 *
 *  _CORE_message_queue_Flush_support
 *
 *  This message handler routine removes all messages from a message queue
 *  and returns them to the inactive message pool.  The number of messages
 *  flushed from the queue is returned
 *
 *  Input parameters:
 *    the_message_queue - pointer to message queue
 *
 *  Output parameters:
 *    returns - number of messages placed on inactive chain
 *
 *  INTERRUPT LATENCY:
 *    only case
 */

unsigned32 _CORE_message_queue_Flush_support(
  CORE_message_queue_Control *the_message_queue
)
{
  ISR_Level   level;
  Chain_Node *inactive_first;
  Chain_Node *message_queue_first;
  Chain_Node *message_queue_last;
  unsigned32  count;

  /*
   *  Currently, RTEMS supports no API that has both flush and blocking
   *  sends.  Thus, this routine assumes that there are no senders
   *  blocked waiting to send messages.  In the event, that an API is
   *  added that can flush a message queue when threads are blocked
   *  waiting to send, there are two basic behaviors envisioned:
   *
   *  (1) The thread queue of pending senders is a logical extension
   *  of the pending message queue.  In this case, it should be 
   *  flushed using the _Thread_queue_Flush() service with a status
   *  such as CORE_MESSAGE_QUEUE_SENDER_FLUSHED (which currently does
   *  not exist).  This can be implemented without changing the "big-O"
   *  of the message flushing part of the routine.
   *
   *  (2) Only the actual messages queued should be purged.  In this case,
   *  the blocked sender threads must be allowed to send their messages.
   *  In this case, the implementation will be forced to individually
   *  dequeue the senders and queue their messages.  This will force
   *  this routine to have "big O(n)" where n is the number of blocked
   *  senders.  If there are more messages pending than senders blocked,
   *  then the existing flush code can be used to dispose of the remaining
   *  pending messages.
   *
   *  For now, though, we are very happy to have a small routine with 
   *  fixed execution time that only deals with pending messages.
   */

  _ISR_Disable( level );
    inactive_first      = the_message_queue->Inactive_messages.first;
    message_queue_first = the_message_queue->Pending_messages.first;
    message_queue_last  = the_message_queue->Pending_messages.last;

    the_message_queue->Inactive_messages.first = message_queue_first;
    message_queue_last->next = inactive_first;
    inactive_first->previous = message_queue_last;
    message_queue_first->previous          =
               _Chain_Head( &the_message_queue->Inactive_messages );

    _Chain_Initialize_empty( &the_message_queue->Pending_messages );

    count = the_message_queue->number_of_pending_messages;
    the_message_queue->number_of_pending_messages = 0;
  _ISR_Enable( level );
  return count;
}

/*PAGE
 *
 *  _CORE_message_queue_Flush_waiting_threads
 *
 *  This function flushes the message_queue's task wait queue.  The number
 *  of messages flushed from the queue is returned.
 *
 *  Input parameters:
 *    the_message_queue - the message_queue to be flushed
 *
 *  Output parameters:
 *    returns - the number of messages flushed from the queue
 */
 
void _CORE_message_queue_Flush_waiting_threads(
  CORE_message_queue_Control *the_message_queue
)
{
  /* XXX this is not supported for global message queues */

  /*
   *  IF there are no pending messages,
   *  THEN threads may be blocked waiting to RECEIVE a message,
   *  
   *  IF the pending message queue is full
   *  THEN threads may be blocked waiting to SEND a message
   *
   *  But in either case, we will return "unsatisfied nowait"
   *  to indicate that the blocking condition was not satisfied
   *  and that the blocking state was canceled.
   */

  _Thread_queue_Flush(
    &the_message_queue->Wait_queue,
    CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED_NOWAIT
  );
}
