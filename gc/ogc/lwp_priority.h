/*  priority.h
 *
 *  This include file contains all thread priority manipulation routines.
 *  This Handler provides mechanisms which can be used to
 *  initialize and manipulate thread priorities.
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

#ifndef __LWP_PRIORITY_H__
#define __LWP_PRIORITY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include "machine/processor.h"

#define PRIORITY_MINIMUM      0         /* highest thread priority */
#define PRIORITY_MAXIMUM      255       /* lowest thread priority */

typedef struct {
	u32 *minor;
	u32 ready_minor,ready_major;
	u32 block_minor,block_major;
} Priority_Information;

extern vu32 _Priority_Major_bit_map;
extern u32 _Priority_Bit_map[];

void _Priority_Handler_initialization();

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_priority.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
