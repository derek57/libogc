/*  sysstates.h
 *
 *  This include file contains information regarding the system state.
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

#ifndef __SYS_STATE_H__
#define __SYS_STATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>

typedef enum {
  SYSTEM_STATE_BEFORE_INITIALIZATION,   /* start -> end of 1st init part */
  SYSTEM_STATE_BEFORE_MULTITASKING,     /* end of 1st -> beginning of 2nd */
  SYSTEM_STATE_BEGIN_MULTITASKING,      /* just before multitasking starts */
  SYSTEM_STATE_UP,                      /* normal operation */
  SYSTEM_STATE_SHUTDOWN,                /* shutdown */
  SYSTEM_STATE_FAILED                   /* fatal error occurred */
} System_state_Codes; 

extern u32 _System_state_Current;

#ifdef __RTEMS_APPLICATION__
#include <libogc/sys_state.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
