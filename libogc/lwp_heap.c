#include <stdlib.h>
#include <system.h>
#include <processor.h>
#include <sys_state.h>
#include <lwp_config.h>

#include "lwp_heap.h"


u32 _Heap_Initialize(Heap_Control *theheap,void *start_addr,u32 size,u32 pg_size)
{
	u32 dsize,level;
	heap_block *block;

	if(!_Heap_Is_page_size_valid(pg_size) || size<HEAP_MIN_SIZE) return 0;

	_CPU_ISR_Disable(level);
	theheap->pg_size = pg_size;
	dsize = (size - HEAP_OVERHEAD);
	
	block = (heap_block*)start_addr;
	block->back_flag = HEAP_DUMMY_FLAG;
	block->front_flag = dsize;
	block->next	= _Heap_Tail(theheap);
	block->prev = _Heap_Head(theheap);
	
	theheap->start = block;
	theheap->first = block;
	theheap->perm_null = NULL;
	theheap->last = block;
	
	block = _Heap_Next_block(block);
	block->back_flag = dsize;
	block->front_flag = HEAP_DUMMY_FLAG;
	theheap->final = block;
	_CPU_ISR_Restore(level);
	
	return (dsize - HEAP_BLOCK_USED_OVERHEAD);
}

void* _Heap_Allocate(Heap_Control *theheap,u32 size)
{
	u32 excess;
	u32 dsize;
	heap_block *block;
	heap_block *next_block;
	heap_block *tmp_block;
	void *ptr;
	u32 offset,level;


	if(size>=(-1-HEAP_BLOCK_USED_OVERHEAD)) return NULL;

	_CPU_ISR_Disable(level);
	excess = (size % theheap->pg_size);
	dsize = (size + theheap->pg_size + HEAP_BLOCK_USED_OVERHEAD);
	
	if(excess)
		dsize += (theheap->pg_size - excess);

	if(dsize<sizeof(heap_block)) dsize = sizeof(heap_block);
	
	for(block=theheap->first;;block=block->next) {
		if(block==_Heap_Tail(theheap)) {
			_CPU_ISR_Restore(level);
			return NULL;
		}
		if(block->front_flag>=dsize) break;
	}
	
	if((block->front_flag-dsize)>(theheap->pg_size+HEAP_BLOCK_USED_OVERHEAD)) {
		block->front_flag -= dsize;
		next_block = _Heap_Next_block(block);
		next_block->back_flag = block->front_flag;
		
		tmp_block = _Heap_Block_at(next_block,dsize);
		tmp_block->back_flag = next_block->front_flag = _Heap_Build_flag(dsize,HEAP_BLOCK_USED);

		ptr = _Heap_Start_of_user_area(next_block);
	} else {
		next_block = _Heap_Next_block(block);
		next_block->back_flag = _Heap_Build_flag(block->front_flag,HEAP_BLOCK_USED);
		
		block->front_flag = next_block->back_flag;
		block->next->prev = block->prev;
		block->prev->next = block->next;
		
		ptr = _Heap_Start_of_user_area(block);
	}

	offset = (theheap->pg_size - ((u32)ptr&(theheap->pg_size-1)));
	ptr += offset;
	*(((u32*)ptr)-1) = offset;
	_CPU_ISR_Restore(level);

	return ptr;
}

BOOL _Heap_Free(Heap_Control *theheap,void *ptr)
{
	heap_block *block;
	heap_block *next_block;
	heap_block *new_next;
	heap_block *prev_block;
	heap_block *tmp_block;
	u32 dsize,level;

	_CPU_ISR_Disable(level);

	block = _Heap_User_block_at(ptr);
	if(!_Heap_Is_block_in(theheap,block) || _Heap_Is_block_free(block)) {
		_CPU_ISR_Restore(level);
		return FALSE;
	}

	dsize = _Heap_Block_size(block);
	next_block = _Heap_Block_at(block,dsize);
	
	if(!_Heap_Is_block_in(theheap,next_block) || (block->front_flag!=next_block->back_flag)) {
		_CPU_ISR_Restore(level);
		return FALSE;
	}
	
	if(_Heap_Is_previous_block_free(block)) {
		prev_block = _Heap_Previous_block(block);
		if(!_Heap_Is_block_in(theheap,prev_block)) {
			_CPU_ISR_Restore(level);
			return FALSE;
		}
		
		if(_Heap_Is_block_free(next_block)) {
			prev_block->front_flag += next_block->front_flag+dsize;
			tmp_block = _Heap_Next_block(prev_block);
			tmp_block->back_flag = prev_block->front_flag;
			next_block->next->prev = next_block->prev;
			next_block->prev->next = next_block->next;
		} else {
			prev_block->front_flag = next_block->back_flag = prev_block->front_flag+dsize;
		}
	} else if(_Heap_Is_block_free(next_block)) {
		block->front_flag = dsize+next_block->front_flag;
		new_next = _Heap_Next_block(block);
		new_next->back_flag = block->front_flag;
		block->next = next_block->next;
		block->prev = next_block->prev;
		next_block->prev->next = block;
		next_block->next->prev = block;
		
		if(theheap->first==next_block) theheap->first = block;
	} else {
		next_block->back_flag = block->front_flag = dsize;
		block->prev = _Heap_Head(theheap);
		block->next = theheap->first;
		theheap->first = block;
		block->next->prev = block;
	}
	_CPU_ISR_Restore(level);

	return TRUE;
}

u32 _Heap_Get_information(Heap_Control *theheap,Heap_Information_block *theinfo)
{
	u32 not_done = 1;
	heap_block *theblock = NULL;
	heap_block *nextblock = NULL;
	
	theinfo->free_blocks = 0;
	theinfo->free_size = 0;
	theinfo->used_blocks = 0;
	theinfo->used_size = 0;
	
	if(!_System_state_Is_up(_System_state_Get())) return 1;

	theblock = theheap->start;
	if(theblock->back_flag!=HEAP_DUMMY_FLAG) return 2;

	while(not_done) {
		if(_Heap_Is_block_free(theblock)) {
			theinfo->free_blocks++;
			theinfo->free_size += _Heap_Block_size(theblock);
		} else {
			theinfo->used_blocks++;
			theinfo->used_size += _Heap_Block_size(theblock);
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
