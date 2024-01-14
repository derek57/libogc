/*
 *  Heap Handler
 *
 *  COPYRIGHT (c) 1989-2006.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id$
 */

#include <stdlib.h>
#include <system.h>
#include <processor.h>
#include <sys_state.h>
#include <lwp_config.h>

#include "lwp_heap.h"


/*PAGE
 *
 *  _Heap_Initialize
 *
 *  This kernel routine initializes a heap.
 *
 *  Input parameters:
 *    the_heap         - pointer to heap header
 *    starting_address - starting address of heap
 *    size             - size of heap
 *    page_size        - allocatable unit of memory
 *
 *  Output parameters:
 *    returns - maximum memory available if RTEMS_SUCCESSFUL
 *    0       - otherwise
 *
 *    This is what a heap looks like in memory immediately
 *    after initialization:
 *
 *            +--------------------------------+
 *     0      |  size = 0      | status = used |  a.k.a.  dummy back flag
 *            +--------------------------------+
 *     4      |  size = size-8 | status = free |  a.k.a.  front flag
 *            +--------------------------------+
 *     8      |  next     = PERM HEAP_TAIL     |
 *            +--------------------------------+
 *    12      |  previous = PERM HEAP_HEAD     |
 *            +--------------------------------+
 *            |                                |
 *            |      memory available          |
 *            |       for allocation           |
 *            |                                |
 *            +--------------------------------+
 *  size - 8  |  size = size-8 | status = free |  a.k.a.  back flag
 *            +--------------------------------+
 *  size - 4  |  size = 0      | status = used |  a.k.a.  dummy front flag
 *            +--------------------------------+
 */

unsigned32 _Heap_Initialize(
  Heap_Control        *the_heap,
  void                *starting_address,
  unsigned32           size,
  unsigned32           page_size
)
{
  Heap_Block        *the_block;
  unsigned32         the_size;
  unsigned32         level;

  if ( !_Heap_Is_page_size_valid( page_size ) ||
       (size < HEAP_MINIMUM_SIZE) )
    return 0;

  _CPU_ISR_Disable(level);
  the_heap->page_size = page_size;
  the_size = size - HEAP_OVERHEAD;

  the_block             = (Heap_Block *) starting_address;
  the_block->back_flag  = HEAP_DUMMY_FLAG;
  the_block->front_flag = the_size;
  the_block->next       = _Heap_Tail( the_heap );
  the_block->previous   = _Heap_Head( the_heap );

  the_heap->start          = the_block;
  the_heap->first          = the_block;
  the_heap->permanent_null = NULL;
  the_heap->last           = the_block;

  the_block             = _Heap_Next_block( the_block );
  the_block->back_flag  = the_size;
  the_block->front_flag = HEAP_DUMMY_FLAG;
  the_heap->final       = the_block;
  _CPU_ISR_Restore(level);

  return ( the_size - HEAP_BLOCK_USED_OVERHEAD );
}

/*PAGE
 *
 *  _Heap_Allocate
 *
 *  This kernel routine allocates the requested size of memory
 *  from the specified heap.
 *
 *  Input parameters:
 *    the_heap  - pointer to heap header.
 *    size      - size in bytes of the memory block to allocate.
 *
 *  Output parameters:
 *    returns - starting address of memory block allocated
 */

void *_Heap_Allocate(
  Heap_Control        *the_heap,
  unsigned32           size
)
{
  unsigned32  excess;
  unsigned32  the_size;
  Heap_Block *the_block;
  Heap_Block *next_block;
  Heap_Block *temporary_block;
  void       *ptr;
  unsigned32  offset;
  unsigned32  level;

  /*
   * Catch the case of a user allocating close to the limit of the
   * unsigned32.
   */

  if ( size >= (-1 - HEAP_BLOCK_USED_OVERHEAD) )
    return( NULL );

  _CPU_ISR_Disable(level);
  excess   = size % the_heap->page_size;
  the_size = size + the_heap->page_size + HEAP_BLOCK_USED_OVERHEAD;
  
  if ( excess )
    the_size += the_heap->page_size - excess;

  if ( the_size < sizeof( Heap_Block ) )
    the_size = sizeof( Heap_Block );

  for ( the_block = the_heap->first;
        ;
        the_block = the_block->next ) {
    if ( the_block == _Heap_Tail( the_heap ) ) {
      _CPU_ISR_Restore(level);
      return( NULL );
    }
    if ( the_block->front_flag >= the_size )
      break;
  }

  if ( (the_block->front_flag - the_size) >
       (the_heap->page_size + HEAP_BLOCK_USED_OVERHEAD) ) {
    the_block->front_flag -= the_size;
    next_block             = _Heap_Next_block( the_block );
    next_block->back_flag  = the_block->front_flag;

    temporary_block            = _Heap_Block_at( next_block, the_size );
    temporary_block->back_flag =
    next_block->front_flag     = _Heap_Build_flag( the_size,
                                    HEAP_BLOCK_USED );
    ptr = _Heap_Start_of_user_area( next_block );
  } else {
    next_block                = _Heap_Next_block( the_block );
    next_block->back_flag     = _Heap_Build_flag( the_block->front_flag,
                                   HEAP_BLOCK_USED );
    the_block->front_flag     = next_block->back_flag;
    the_block->next->previous = the_block->previous;
    the_block->previous->next = the_block->next;
    ptr = _Heap_Start_of_user_area( the_block );
  }
  
  /*
   * round ptr up to a multiple of page size
   * Have to save the bump amount in the buffer so that free can figure it out
   */
  
  offset = the_heap->page_size - (((unsigned32) ptr) & (the_heap->page_size - 1));
  ptr += offset;
  *(((unsigned32 *) ptr) - 1) = offset;

#ifdef RTEMS_DEBUG
  {
      unsigned32 ptr_u32;
      ptr_u32 = (unsigned32) ptr;
      if (ptr_u32 & (the_heap->page_size - 1))
          abort();
  }
#endif

  _CPU_ISR_Restore(level);

  return ptr;
}

/*PAGE
 *
 *  _Heap_Free
 *
 *  This kernel routine returns the memory designated by the
 *  given heap and given starting address to the memory pool.
 *
 *  Input parameters:
 *    the_heap         - pointer to heap header
 *    starting_address - starting address of the memory block to free.
 *
 *  Output parameters:
 *    TRUE  - if starting_address is valid heap address
 *    FALSE - if starting_address is invalid heap address
 */

boolean _Heap_Free(
  Heap_Control        *the_heap,
  void                *starting_address
)
{
  Heap_Block        *the_block;
  Heap_Block        *next_block;
  Heap_Block        *new_next_block;
  Heap_Block        *previous_block;
  Heap_Block        *temporary_block;
  unsigned32         the_size;
  unsigned32         level;

  _CPU_ISR_Disable(level);

  the_block = _Heap_User_block_at( starting_address );

  if ( !_Heap_Is_block_in( the_heap, the_block ) ||
        _Heap_Is_block_free( the_block ) ) {
      _CPU_ISR_Restore(level);
      return( FALSE );
  }

  the_size   = _Heap_Block_size( the_block );
  next_block = _Heap_Block_at( the_block, the_size );

  if ( !_Heap_Is_block_in( the_heap, next_block ) ||
       (the_block->front_flag != next_block->back_flag) ) {
      _CPU_ISR_Restore(level);
      return( FALSE );
  }

  if ( _Heap_Is_previous_block_free( the_block ) ) {
    previous_block = _Heap_Previous_block( the_block );

    if ( !_Heap_Is_block_in( the_heap, previous_block ) ) {
        _CPU_ISR_Restore(level);
        return( FALSE );
    }

    if ( _Heap_Is_block_free( next_block ) ) {      /* coalesce both */
      previous_block->front_flag += next_block->front_flag + the_size;
      temporary_block             = _Heap_Next_block( previous_block );
      temporary_block->back_flag  = previous_block->front_flag;
      next_block->next->previous  = next_block->previous;
      next_block->previous->next  = next_block->next;
    }
    else {                     /* coalesce prev */
      previous_block->front_flag =
      next_block->back_flag      = previous_block->front_flag + the_size;
    }
  }
  else if ( _Heap_Is_block_free( next_block ) ) { /* coalesce next */
    the_block->front_flag     = the_size + next_block->front_flag;
    new_next_block            = _Heap_Next_block( the_block );
    new_next_block->back_flag = the_block->front_flag;
    the_block->next           = next_block->next;
    the_block->previous       = next_block->previous;
    next_block->previous->next = the_block;
    next_block->next->previous = the_block;

    if (the_heap->first == next_block)
        the_heap->first = the_block;
  }
  else {                          /* no coalesce */
    next_block->back_flag     =
    the_block->front_flag     = the_size;
    the_block->previous       = _Heap_Head( the_heap );
    the_block->next           = the_heap->first;
    the_heap->first           = the_block;
    the_block->next->previous = the_block;
  }
  _CPU_ISR_Restore(level);

  return( TRUE );
}

/*PAGE
 *
 *  _Heap_Get_information
 *
 *  This kernel routine walks the heap and tots up the free and allocated
 *  sizes.  Derived from _Heap_Walk.
 *
 *  Input parameters:
 *    the_heap  - pointer to heap header
 *    the_info  - pointer for information to be returned
 *
 *  Output parameters: 
 *    *the_info  - contains information about heap
 *    return 0=success, otherwise heap is corrupt.
 */

Heap_Get_information_status _Heap_Get_information(
  Heap_Control            *the_heap,
  Heap_Information_block  *the_info
)
{
  Heap_Block *the_block  = 0;  /* avoid warnings */
  Heap_Block *next_block = 0;  /* avoid warnings */
  int         notdone = 1;


  the_info->free_blocks = 0;
  the_info->free_size   = 0;
  the_info->used_blocks = 0;
  the_info->used_size   = 0;

  /*
   * We don't want to allow walking the heap until we have
   * transferred control to the user task so we watch the
   * system state.
   */

  if ( !_System_state_Is_up( _System_state_Get() ) )
    return HEAP_GET_INFORMATION_SYSTEM_STATE_ERROR;

  the_block = the_heap->start;

  /*
   * Handle the 1st block
   */

  if ( the_block->back_flag != HEAP_DUMMY_FLAG ) {
    return HEAP_GET_INFORMATION_BLOCK_ERROR;
  }

  while (notdone) {
      
      /*
       * Accumulate size
       */

      if ( _Heap_Is_block_free(the_block) ) {
        the_info->free_blocks++;
        the_info->free_size += _Heap_Block_size(the_block);
      } else {
        the_info->used_blocks++;
        the_info->used_size += _Heap_Block_size(the_block);
      }
    
      /*
       * Handle the last block
       */

      if ( the_block->front_flag != HEAP_DUMMY_FLAG ) {
        next_block = _Heap_Next_block(the_block);
        if ( the_block->front_flag != next_block->back_flag ) {
          return HEAP_GET_INFORMATION_BLOCK_ERROR;
        }
      }

      if ( the_block->front_flag == HEAP_DUMMY_FLAG )
        notdone = 0;
      else
        the_block = next_block;
  }  /* while(notdone) */

  return HEAP_GET_INFORMATION_SUCCESSFUL;
}
