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

/*
 *  The following record defines the information associated with
 *  each thread to manage its interaction with the priority bit maps.
 */

typedef struct {
  Priority_Bit_map_control *minor;        /* addr of minor bit map slot */
  Priority_Bit_map_control  ready_minor;  /* priority bit map ready mask */
  Priority_Bit_map_control  ready_major;  /* priority bit map ready mask */
  Priority_Bit_map_control  block_minor;  /* priority bit map block mask */
  Priority_Bit_map_control  block_major;  /* priority bit map block mask */
}   Priority_Information;

/*
 *  The following data items are the priority bit map.
 *  Each of the sixteen bits used in the _Priority_Major_bit_map is
 *  associated with one of the sixteen entries in the _Priority_Bit_map.
 *  Each bit in the _Priority_Bit_map indicates whether or not there are
 *  threads ready at a particular priority.  The mapping of
 *  individual priority levels to particular bits is processor
 *  dependent as is the value of each bit used to indicate that
 *  threads are ready at that priority.
 */

SCORE_EXTERN volatile Priority_Bit_map_control _Priority_Major_bit_map;
SCORE_EXTERN Priority_Bit_map_control 
               _Priority_Bit_map[16] CPU_STRUCTURE_ALIGNMENT;

void _Priority_Handler_initialization( void );

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_priority.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
