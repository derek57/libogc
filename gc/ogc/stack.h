/*  stack.h
 *
 *  This include file contains all information about the thread
 *  Stack Handler.  This Handler provides mechanisms which can be used to
 *  initialize and utilize stacks.
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

#ifndef __STACK_h
#define __STACK_h

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <thread.h>

/*
 *  This number corresponds to the byte alignment requirement for the
 *  stack.  This alignment requirement may be stricter than that for the
 *  data types alignment specified by CPU_ALIGNMENT.  If the CPU_ALIGNMENT
 *  is strict enough for the stack, then this should be set to 0.
 *
 *  NOTE:  This must be a power of 2 either 0 or greater than CPU_ALIGNMENT.
 */

#define CPU_STACK_ALIGNMENT             (PPC_STACK_ALIGNMENT)

/*
 *  Should be large enough to run all RTEMS tests.  This insures
 *  that a "reasonable" small application should not have any problems.
 */

#define CPU_STACK_MINIMUM_SIZE          (1024*8)

/*
 * Needed for Interrupt stack
 */

//#define CPU_MINIMUM_STACK_FRAME_SIZE    8 // ??? - "libOGC" uses 16
#define CPU_MINIMUM_STACK_FRAME_SIZE	16  // ??? - but it should be 8

/*
 *  This defines the number of levels and the mask used to pick those
 *  bits out of a thread mode.
 */

#define CPU_MODES_INTERRUPT_MASK        0x00000001 /* interrupt level in mode */

/*
 *  The following constant defines the minimum stack size which every
 *  thread must exceed.
 */

#define STACK_MINIMUM_SIZE              CPU_STACK_MINIMUM_SIZE

/*
 *  _Thread_Stack_Allocate
 *  
 *  DESCRIPTION:
 *
 *  Allocate the requested stack space for the thread.
 *  return the actual size allocated after any adjustment
 *  or return zero if the allocation failed.
 *  Set the Start.stack field to the address of the stack
 *
 *  NOTES: NONE
 *
 */

unsigned32 _Thread_Stack_Allocate(
  Thread_Control *the_thread,
  unsigned32 stack_size
);

/*
 *  _Thread_Stack_Free
 *
 *  DESCRIPTION:
 *
 *  Deallocate the Thread's stack.
 *  NOTES: NONE
 *
 */

void _Thread_Stack_Free(
  Thread_Control *the_thread
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/stack.inl>
#endif
	
#ifdef __cplusplus
	}
#endif

#endif
