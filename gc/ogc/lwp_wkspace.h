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

#ifndef __LWP_WKSPACE_H__
#define __LWP_WKSPACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <lwp_heap.h>

extern Heap_Control _Workspace_Area;

/*
 *  _Workspace_Handler_initialization
 *
 *  DESCRIPTION:
 *
 *  This routine performs the initialization necessary for this handler.
 */

void _Workspace_Handler_initialization(u32 size);

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_wkspace.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
