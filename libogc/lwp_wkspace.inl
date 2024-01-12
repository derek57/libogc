#ifndef __LWP_WKSPACE_INL__
#define __LWP_WKSPACE_INL__

static __inline__ void* _Workspace_Allocate(u32 size)
{
	return _Heap_Allocate(&_Workspace_Area,size);
}

static __inline__ BOOL _Workspace_Free(void *block)
{
	return _Heap_Free(&_Workspace_Area,block);
}

#endif
