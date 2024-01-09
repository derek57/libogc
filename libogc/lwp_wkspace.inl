#ifndef __LWP_WKSPACE_INL__
#define __LWP_WKSPACE_INL__

static __inline__ void* _Workspace_Allocate(u32 size)
{
	return _Heap_Allocate(&__wkspace_heap,size);
}

static __inline__ BOOL _Workspace_Free(void *ptr)
{
	return _Heap_Free(&__wkspace_heap,ptr);
}

#endif
