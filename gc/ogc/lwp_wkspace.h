#ifndef __LWP_WKSPACE_H__
#define __LWP_WKSPACE_H__

#include <gctypes.h>
#include <lwp_heap.h>

#ifdef __cplusplus
extern "C" {
#endif

extern Heap_Control _Workspace_Area;

void _Workspace_Handler_initialization(u32 size);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_wkspace.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
