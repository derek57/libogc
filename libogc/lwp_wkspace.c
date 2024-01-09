#include <stdlib.h>
#include <system.h>
#include <string.h>
#include <asm.h>
#include <processor.h>
#include "system.h"
#include "lwp_wkspace.h"

#define ROUND32UP(v)			(((u32)(v)+31)&~31)

heap_cntrl __wkspace_heap;
static u32 __wkspace_heap_size = 0;

void _Workspace_Handler_initialization(u32 size)
{
	u32 arLo,level,dsize;

	// Get current ArenaLo and adjust to 32-byte boundary
	_CPU_ISR_Disable(level);
	arLo = ROUND32UP(SYS_GetArenaLo());
	dsize = (size - (arLo - (u32)SYS_GetArenaLo()));
	SYS_SetArenaLo((void*)(arLo+dsize));
	_CPU_ISR_Restore(level);

	memset((void*)arLo,0,dsize);
	__wkspace_heap_size += _Heap_Initialize(&__wkspace_heap,(void*)arLo,dsize,PPC_ALIGNMENT);
}
