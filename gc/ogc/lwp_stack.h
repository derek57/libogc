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

#ifndef __LWP_STACK_H__
#define __LWP_STACK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <lwp_threads.h>

#define CPU_STACK_ALIGNMENT				8
#define CPU_STACK_MINIMUM_SIZE			1024*8
#define CPU_MINIMUM_STACK_FRAME_SIZE	16
#define CPU_MODES_INTERRUPT_MASK		0x00000001 /* interrupt level in mode */

/*
 *  The following constant defines the minimum stack size which every
 *  thread must exceed.
 */

#define STACK_MINIMUM_SIZE  CPU_STACK_MINIMUM_SIZE

u32 _Thread_Stack_Allocate(Thread_Control *,u32);
void _Thread_Stack_Free(Thread_Control *);

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_stack.inl>
#endif
	
#ifdef __cplusplus
	}
#endif

#endif
