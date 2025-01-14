/*  wkspace.h
 *
 *  This include file contains information related to the 
 *  RAM Workspace.  This Handler provides mechanisms which can be used to
 *  define, initialize and manipulate the workspace.
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

#ifndef __WORKSPACE_h
#define __WORKSPACE_h

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <heap.h>

/*
 *  The following is used to manage the Workspace.
 *
 */

SCORE_EXTERN Heap_Control _Workspace_Area;  /* executive heap header */

/*
 *  _Workspace_Handler_initialization
 *
 *  DESCRIPTION:
 *
 *  This routine performs the initialization necessary for this handler.
 */

void _Workspace_Handler_initialization(
  unsigned32  size
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/wkspace.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
