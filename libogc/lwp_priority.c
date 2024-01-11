#include <gctypes.h>

vu32 _Priority_Major_bit_map;
u32 _Priority_Bit_map[16] __attribute__((aligned(32)));

void _Priority_Handler_initialization()
{
	u32 index;
	
	_Priority_Major_bit_map = 0;
	for(index=0;index<16;index++)
		_Priority_Bit_map[index] = 0;
	
}
