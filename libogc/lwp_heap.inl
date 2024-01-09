#ifndef __LWP_HEAP_INL__
#define __LWP_HEAP_INL__

static __inline__ heap_block* _Heap_Head(heap_cntrl *theheap)
{
	return (heap_block*)&theheap->start;
}

static __inline__ heap_block* _Heap_Tail(heap_cntrl *heap)
{
	return (heap_block*)&heap->final;
}

static __inline__ heap_block* _Heap_Previous_block(heap_block *block)
{
	return (heap_block*)((char*)block - (block->back_flag&~HEAP_BLOCK_USED));
}

static __inline__ heap_block* _Heap_Next_block(heap_block *block)
{
	return (heap_block*)((char*)block + (block->front_flag&~HEAP_BLOCK_USED));
}

static __inline__ heap_block* _Heap_Block_at(heap_block *block,u32 offset)
{
	return (heap_block*)((char*)block + offset);
}

static __inline__ heap_block* _Heap_User_block_at(void *ptr)
{
	u32 offset = *(((u32*)ptr)-1);
	return _Heap_Block_at(ptr,-offset+-HEAP_BLOCK_USED_OVERHEAD);
}

static __inline__ bool _Heap_Is_previous_block_free(heap_block *block)
{
	return !(block->back_flag&HEAP_BLOCK_USED);
}

static __inline__ bool _Heap_Is_block_free(heap_block *block)
{
	return !(block->front_flag&HEAP_BLOCK_USED);
}

static __inline__ bool _Heap_Is_block_used(heap_block *block)
{
	return (block->front_flag&HEAP_BLOCK_USED);
}

static __inline__ u32 _Heap_Block_size(heap_block *block)
{
	return (block->front_flag&~HEAP_BLOCK_USED);
}

static __inline__ void* _Heap_Start_of_user_area(heap_block *block)
{
	return (void*)&block->next;
}

static __inline__ bool _Heap_Is_block_in(heap_cntrl *heap,heap_block *block)
{
	return ((u32)block>=(u32)heap->start && (u32)block<=(u32)heap->final);
}

static __inline__ bool _Heap_Is_page_size_valid(u32 pgsize)
{
	return (pgsize!=0 && ((pgsize%PPC_ALIGNMENT)==0));
}

static __inline__ u32 _Heap_Build_flag(u32 size,u32 flag)
{
	return (size|flag);
}

#endif
