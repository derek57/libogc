#include <stdlib.h>
#include <system.h>
#include <string.h>
#include <asm.h>
#include <processor.h>
#include "system.h"
#include "lwp_wkspace.h"

#define ROUND32UP(v)			(((u32)(v)+31)&~31)

Heap_Control _Workspace_Area;
static u32 memory_available = 0;

void _Workspace_Handler_initialization(u32 size)
{
	u32 starting_address,level,dsize;

	// Get current ArenaLo and adjust to 32-byte boundary
	_CPU_ISR_Disable(level);
	starting_address = ROUND32UP(SYS_GetArenaLo());
	dsize = (size - (starting_address - (u32)SYS_GetArenaLo()));
	SYS_SetArenaLo((void*)(starting_address+dsize));
	_CPU_ISR_Restore(level);

	memset((void*)starting_address,0,dsize);
	memory_available += _Heap_Initialize(&_Workspace_Area,(void*)starting_address,dsize,PPC_ALIGNMENT);
}
