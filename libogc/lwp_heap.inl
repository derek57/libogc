#ifndef __LWP_HEAP_INL__
#define __LWP_HEAP_INL__

static __inline__ heap_block* _Heap_Head(Heap_Control *the_heap)
{
	return (heap_block*)&the_heap->start;
}

static __inline__ heap_block* _Heap_Tail(Heap_Control *the_heap)
{
	return (heap_block*)&the_heap->final;
}

static __inline__ heap_block* _Heap_Previous_block(heap_block *the_block)
{
	return (heap_block*)((char*)the_block - (the_block->back_flag&~HEAP_BLOCK_USED));
}

static __inline__ heap_block* _Heap_Next_block(heap_block *the_block)
{
	return (heap_block*)((char*)the_block + (the_block->front_flag&~HEAP_BLOCK_USED));
}

static __inline__ heap_block* _Heap_Block_at(heap_block *base,u32 offset)
{
	return (heap_block*)((char*)base + offset);
}

static __inline__ heap_block* _Heap_User_block_at(void *base)
{
	u32 offset = *(((u32*)base)-1);
	return _Heap_Block_at(base,-offset+-HEAP_BLOCK_USED_OVERHEAD);
}

static __inline__ bool _Heap_Is_previous_block_free(heap_block *the_block)
{
	return !(the_block->back_flag&HEAP_BLOCK_USED);
}

static __inline__ bool _Heap_Is_block_free(heap_block *the_block)
{
	return !(the_block->front_flag&HEAP_BLOCK_USED);
}

static __inline__ bool _Heap_Is_block_used(heap_block *the_block)
{
	return (the_block->front_flag&HEAP_BLOCK_USED);
}

static __inline__ u32 _Heap_Block_size(heap_block *the_block)
{
	return (the_block->front_flag&~HEAP_BLOCK_USED);
}

static __inline__ void* _Heap_Start_of_user_area(heap_block *the_block)
{
	return (void*)&the_block->next;
}

static __inline__ bool _Heap_Is_block_in(Heap_Control *the_heap,heap_block *the_block)
{
	return ((u32)the_block>=(u32)the_heap->start && (u32)the_block<=(u32)the_heap->final);
}

static __inline__ bool _Heap_Is_page_size_valid(u32 page_size)
{
	return (page_size!=0 && ((page_size%PPC_ALIGNMENT)==0));
}

static __inline__ u32 _Heap_Build_flag(u32 size,u32 in_use_flag)
{
	return (size|in_use_flag);
}

#endif
