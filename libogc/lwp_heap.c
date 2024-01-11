#include <stdlib.h>
#include <system.h>
#include <processor.h>
#include <sys_state.h>
#include <lwp_config.h>

#include "lwp_heap.h"


u32 _Heap_Initialize(Heap_Control *the_heap,void *starting_address,u32 size,u32 page_size)
{
	u32 the_size,level;
	heap_block *the_block;

	if(!_Heap_Is_page_size_valid(page_size) || size<HEAP_MIN_SIZE) return 0;

	_CPU_ISR_Disable(level);
	the_heap->page_size = page_size;
	the_size = (size - HEAP_OVERHEAD);
	
	the_block = (heap_block*)starting_address;
	the_block->back_flag = HEAP_DUMMY_FLAG;
	the_block->front_flag = the_size;
	the_block->next	= _Heap_Tail(the_heap);
	the_block->previous = _Heap_Head(the_heap);
	
	the_heap->start = the_block;
	the_heap->first = the_block;
	the_heap->permanent_null = NULL;
	the_heap->last = the_block;
	
	the_block = _Heap_Next_block(the_block);
	the_block->back_flag = the_size;
	the_block->front_flag = HEAP_DUMMY_FLAG;
	the_heap->final = the_block;
	_CPU_ISR_Restore(level);
	
	return (the_size - HEAP_BLOCK_USED_OVERHEAD);
}

void* _Heap_Allocate(Heap_Control *the_heap,u32 size)
{
	u32 excess;
	u32 the_size;
	heap_block *the_block;
	heap_block *next_block;
	heap_block *temporary_block;
	void *ptr;
	u32 offset,level;


	if(size>=(-1-HEAP_BLOCK_USED_OVERHEAD)) return NULL;

	_CPU_ISR_Disable(level);
	excess = (size % the_heap->page_size);
	the_size = (size + the_heap->page_size + HEAP_BLOCK_USED_OVERHEAD);
	
	if(excess)
		the_size += (the_heap->page_size - excess);

	if(the_size<sizeof(heap_block)) the_size = sizeof(heap_block);
	
	for(the_block=the_heap->first;;the_block=the_block->next) {
		if(the_block==_Heap_Tail(the_heap)) {
			_CPU_ISR_Restore(level);
			return NULL;
		}
		if(the_block->front_flag>=the_size) break;
	}
	
	if((the_block->front_flag-the_size)>(the_heap->page_size+HEAP_BLOCK_USED_OVERHEAD)) {
		the_block->front_flag -= the_size;
		next_block = _Heap_Next_block(the_block);
		next_block->back_flag = the_block->front_flag;
		
		temporary_block = _Heap_Block_at(next_block,the_size);
		temporary_block->back_flag = next_block->front_flag = _Heap_Build_flag(the_size,HEAP_BLOCK_USED);

		ptr = _Heap_Start_of_user_area(next_block);
	} else {
		next_block = _Heap_Next_block(the_block);
		next_block->back_flag = _Heap_Build_flag(the_block->front_flag,HEAP_BLOCK_USED);
		
		the_block->front_flag = next_block->back_flag;
		the_block->next->previous = the_block->previous;
		the_block->previous->next = the_block->next;
		
		ptr = _Heap_Start_of_user_area(the_block);
	}

	offset = (the_heap->page_size - ((u32)ptr&(the_heap->page_size-1)));
	ptr += offset;
	*(((u32*)ptr)-1) = offset;
	_CPU_ISR_Restore(level);

	return ptr;
}

BOOL _Heap_Free(Heap_Control *the_heap,void *starting_address)
{
	heap_block *the_block;
	heap_block *next_block;
	heap_block *new_next_block;
	heap_block *previous_block;
	heap_block *temporary_block;
	u32 the_size,level;

	_CPU_ISR_Disable(level);

	the_block = _Heap_User_block_at(starting_address);
	if(!_Heap_Is_block_in(the_heap,the_block) || _Heap_Is_block_free(the_block)) {
		_CPU_ISR_Restore(level);
		return FALSE;
	}

	the_size = _Heap_Block_size(the_block);
	next_block = _Heap_Block_at(the_block,the_size);
	
	if(!_Heap_Is_block_in(the_heap,next_block) || (the_block->front_flag!=next_block->back_flag)) {
		_CPU_ISR_Restore(level);
		return FALSE;
	}
	
	if(_Heap_Is_previous_block_free(the_block)) {
		previous_block = _Heap_Previous_block(the_block);
		if(!_Heap_Is_block_in(the_heap,previous_block)) {
			_CPU_ISR_Restore(level);
			return FALSE;
		}
		
		if(_Heap_Is_block_free(next_block)) {
			previous_block->front_flag += next_block->front_flag+the_size;
			temporary_block = _Heap_Next_block(previous_block);
			temporary_block->back_flag = previous_block->front_flag;
			next_block->next->previous = next_block->previous;
			next_block->previous->next = next_block->next;
		} else {
			previous_block->front_flag = next_block->back_flag = previous_block->front_flag+the_size;
		}
	} else if(_Heap_Is_block_free(next_block)) {
		the_block->front_flag = the_size+next_block->front_flag;
		new_next_block = _Heap_Next_block(the_block);
		new_next_block->back_flag = the_block->front_flag;
		the_block->next = next_block->next;
		the_block->previous = next_block->previous;
		next_block->previous->next = the_block;
		next_block->next->previous = the_block;
		
		if(the_heap->first==next_block) the_heap->first = the_block;
	} else {
		next_block->back_flag = the_block->front_flag = the_size;
		the_block->previous = _Heap_Head(the_heap);
		the_block->next = the_heap->first;
		the_heap->first = the_block;
		the_block->next->previous = the_block;
	}
	_CPU_ISR_Restore(level);

	return TRUE;
}

u32 _Heap_Get_information(Heap_Control *the_heap,Heap_Information_block *the_info)
{
	u32 not_done = 1;
	heap_block *theblock = NULL;
	heap_block *nextblock = NULL;
	
	the_info->free_blocks = 0;
	the_info->free_size = 0;
	the_info->used_blocks = 0;
	the_info->used_size = 0;
	
	if(!_System_state_Is_up(_System_state_Get())) return 1;

	theblock = the_heap->start;
	if(theblock->back_flag!=HEAP_DUMMY_FLAG) return 2;

	while(not_done) {
		if(_Heap_Is_block_free(theblock)) {
			the_info->free_blocks++;
			the_info->free_size += _Heap_Block_size(theblock);
		} else {
			the_info->used_blocks++;
			the_info->used_size += _Heap_Block_size(theblock);
		}

		if(theblock->front_flag!=HEAP_DUMMY_FLAG) {
			nextblock = _Heap_Next_block(theblock);
			if(theblock->front_flag!=nextblock->back_flag) return 2;
		}

		if(theblock->front_flag==HEAP_DUMMY_FLAG) 
			not_done = 0;
		else
			theblock = nextblock;
	}
	return 0;
}
