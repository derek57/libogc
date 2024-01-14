#ifndef __LWP_HEAP_INL__
#define __LWP_HEAP_INL__

static __inline__ Heap_Block* _Heap_Head(Heap_Control *the_heap)
{
	return (Heap_Block*)&the_heap->start;
}

static __inline__ Heap_Block* _Heap_Tail(Heap_Control *the_heap)
{
	return (Heap_Block*)&the_heap->final;
}

static __inline__ Heap_Block* _Heap_Previous_block(Heap_Block *the_block)
{
	return (Heap_Block*)((char*)the_block - (the_block->back_flag&~HEAP_BLOCK_USED));
}

static __inline__ Heap_Block* _Heap_Next_block(Heap_Block *the_block)
{
	return (Heap_Block*)((char*)the_block + (the_block->front_flag&~HEAP_BLOCK_USED));
}

static __inline__ Heap_Block* _Heap_Block_at(Heap_Block *base,u32 offset)
{
	return (Heap_Block*)((char*)base + offset);
}

static __inline__ Heap_Block* _Heap_User_block_at(void *base)
{
	u32 offset = *(((u32*)base)-1);
	return _Heap_Block_at(base,-offset+-HEAP_BLOCK_USED_OVERHEAD);
}

static __inline__ bool _Heap_Is_previous_block_free(Heap_Block *the_block)
{
	return !(the_block->back_flag&HEAP_BLOCK_USED);
}

static __inline__ bool _Heap_Is_block_free(Heap_Block *the_block)
{
	return !(the_block->front_flag&HEAP_BLOCK_USED);
}

static __inline__ bool _Heap_Is_block_used(Heap_Block *the_block)
{
	return (the_block->front_flag&HEAP_BLOCK_USED);
}

static __inline__ u32 _Heap_Block_size(Heap_Block *the_block)
{
	return (the_block->front_flag&~HEAP_BLOCK_USED);
}

static __inline__ void* _Heap_Start_of_user_area(Heap_Block *the_block)
{
	return (void*)&the_block->next;
}

static __inline__ bool _Heap_Is_block_in(Heap_Control *the_heap,Heap_Block *the_block)
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
