/*
 *  Thread Handler
 *
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 */

#include <stdlib.h>
#include "lwp_stack.h"
#include "lwp_wkspace.h"

/*PAGE
 *
 *  _Thread_Stack_Allocate
 *
 *  Allocate the requested stack space for the thread.
 *  return the actual size allocated after any adjustment
 *  or return zero if the allocation failed.
 *  Set the Start.stack field to the address of the stack
 */

unsigned32 _Thread_Stack_Allocate(
  Thread_Control *the_thread,
  unsigned32 stack_size
)
{
  void *stack_addr = 0;
 
  if ( !_Stack_Is_enough( stack_size ) )
    stack_size = STACK_MINIMUM_SIZE;
 
  /*
   *  First pad the requested size so we allocate enough memory
   *  so the context initialization can align it properly.  The address
   *  returned the workspace allocate must be directly stored in the
   *  stack control block because it is later used in the free sequence.
   *
   *  Thus it is the responsibility of the CPU dependent code to 
   *  get and keep the stack adjust factor, the stack alignment, and 
   *  the context initialization sequence in sync.
   */

  stack_size = _Stack_Adjust_size( stack_size );
  stack_addr = _Workspace_Allocate( stack_size );
 
  if ( !stack_addr )
      stack_size = 0;
 
  the_thread->stack = stack_addr;
 
  return stack_size;
}

/*
 *  _Thread_Stack_Free
 *
 *  Deallocate the Thread's stack.
 */

void _Thread_Stack_Free(
  Thread_Control *the_thread
)
{
    /*
     *  If the API provided the stack space, then don't free it.
     */

    if ( !the_thread->core_allocated_stack )
      return;

    _Workspace_Free( the_thread->stack );
}
