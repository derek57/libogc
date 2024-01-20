/*
 *  Initialization Manager
 *
 *  COPYRIGHT (c) 1989-1998.
 *  On-Line Applications Research Corporation (OAR).
 *  Copyright assigned to U.S. Government, 1994.
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 */

#include <gctypes.h>

#include "lwp_config.h"

/*
 *  The following data items are the priority bit map.
 *  Each of the sixteen bits used in the _Priority_Major_bit_map is
 *  associated with one of the sixteen entries in the _Priority_Bit_map.
 *  Each bit in the _Priority_Bit_map indicates whether or not there are
 *  threads ready at a particular priority.  The mapping of
 *  individual priority levels to particular bits is processor
 *  dependent as is the value of each bit used to indicate that
 *  threads are ready at that priority.
 */

volatile Priority_Bit_map_control _Priority_Major_bit_map;
Priority_Bit_map_control 
               _Priority_Bit_map[16] CPU_STRUCTURE_ALIGNMENT;

/*PAGE
 *
 *  _Priority_Handler_initialization
 *
 *  DESCRIPTION:
 *
 *  This routine performs the initialization necessary for this handler.
 */

void _Priority_Handler_initialization( void )
{
  unsigned32 index;

  _Priority_Major_bit_map = 0;
  for ( index=0 ; index <16 ; index++ )
     _Priority_Bit_map[ index ] = 0;
}
