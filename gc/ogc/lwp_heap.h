#ifndef __LWP_HEAP_H__
#define __LWP_HEAP_H__

#include <gctypes.h>
#include "machine/asm.h"

#define HEAP_BLOCK_USED					1
#define HEAP_BLOCK_FREE					0

#define HEAP_DUMMY_FLAG					(0+HEAP_BLOCK_USED)

#define HEAP_OVERHEAD					(sizeof(u32)*2)
#define HEAP_BLOCK_USED_OVERHEAD		(sizeof(void*)*2)
#define HEAP_MIN_SIZE					(HEAP_OVERHEAD+sizeof(heap_block))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Heap_Block_struct heap_block;
struct Heap_Block_struct {
	u32 back_flag;
	u32 front_flag;
	heap_block *next;
	heap_block *previous;
};

typedef struct {
	u32 free_blocks;
	u32 free_size;
	u32 used_blocks;
	u32 used_size;
} Heap_Information_block;

typedef struct {
	heap_block *start;
	heap_block *final;

	heap_block *first;
	heap_block *permanent_null;
	heap_block *last;
	u32 page_size;
	u32 reserved;
} Heap_Control;

u32 _Heap_Initialize(Heap_Control *theheap,void *start_addr,u32 size,u32 pg_size);
void* _Heap_Allocate(Heap_Control *theheap,u32 size);
BOOL _Heap_Free(Heap_Control *theheap,void *ptr);
u32 _Heap_Get_information(Heap_Control *theheap,Heap_Information_block *theinfo);

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_heap.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
