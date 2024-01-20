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

#ifndef __RTEMS_SYSTEM_STATE_h
#define __RTEMS_SYSTEM_STATE_h

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>

#include "lwp_config.h"

/* enumerated constants */

/*
 *  The following type defines the possible system states.
 */

typedef enum {
  SYSTEM_STATE_BEFORE_INITIALIZATION,   /* start -> end of 1st init part */
  SYSTEM_STATE_BEFORE_MULTITASKING,     /* end of 1st -> beginning of 2nd */
  SYSTEM_STATE_BEGIN_MULTITASKING,      /* just before multitasking starts */
  SYSTEM_STATE_UP,                      /* normal operation */
  SYSTEM_STATE_SHUTDOWN,                /* shutdown */
  SYSTEM_STATE_FAILED                   /* fatal error occurred */
} System_state_Codes; 

/*
 *  The following variable contains the current system state.
 */

SCORE_EXTERN System_state_Codes _System_state_Current;

#ifdef __RTEMS_APPLICATION__
#include <libogc/sysstate.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
