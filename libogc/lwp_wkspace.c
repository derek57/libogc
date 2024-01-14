/*
 *  Workspace Handler
 *
 *  XXX
 *
 *  NOTE:
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 */

#include <stdlib.h>
#include <system.h>
#include <string.h>
#include <asm.h>
#include <processor.h>
#include "system.h"
#include "lwp_wkspace.h"

#define ROUND32UP(v)			(((u32)(v)+31)&~31)

Heap_Control _Workspace_Area;
static Heap_Information_block __wkspace_iblock;
static u32 memory_available = 0;

u32 __lwp_wkspace_heapsize()
{
	return memory_available;
}

u32 __lwp_wkspace_heapfree()
{
	_Heap_Get_information(&_Workspace_Area,&__wkspace_iblock);
	return __wkspace_iblock.free_size;
}

u32 __lwp_wkspace_heapused()
{
	_Heap_Get_information(&_Workspace_Area,&__wkspace_iblock);
	return __wkspace_iblock.used_size;
}

/*PAGE
 *
 *  _Workspace_Handler_initialization
 */

void _Workspace_Handler_initialization(u32 size)
{
	u32 starting_address,level,dsize;

	// Get current ArenaLo and adjust to 32-byte boundary
	_ISR_Disable(level);
	starting_address = ROUND32UP(SYS_GetArenaLo());
	dsize = (size - (starting_address - (u32)SYS_GetArenaLo()));
	SYS_SetArenaLo((void*)(starting_address+dsize));
	_ISR_Enable(level);

	memset((void*)starting_address,0,dsize);
	memory_available += _Heap_Initialize(&_Workspace_Area,(void*)starting_address,dsize,CPU_HEAP_ALIGNMENT);
}
