/*  priority.inl
 *
 *  This file contains the static inline implementation of all inlined
 *  routines in the Priority Handler.
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

#ifndef __LWP_PRIORITY_INL__
#define __LWP_PRIORITY_INL__

/*PAGE
 *
 *  _Priority_Major
 *
 */

#define _Priority_Major( _the_priority ) ( (_the_priority) / 16 )

/*PAGE
 *
 *  _Priority_Minor
 *
 */

#define _Priority_Minor( _the_priority ) ( (_the_priority) % 16 )

/*PAGE
 *
 *  _Priority_Initialize_information
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the_priority_map so that it
 *  contains the information necessary to manage a thread
 *  at new_priority.
 */

RTEMS_INLINE_ROUTINE void _Priority_Initialize_information(
  Priority_Information *the_priority_map,
  Priority_Control      new_priority
)
{
  Priority_Bit_map_control major;
  Priority_Bit_map_control minor;
  Priority_Bit_map_control mask;

  major = _Priority_Major( new_priority );
  minor = _Priority_Minor( new_priority );

  the_priority_map->minor =
    &_Priority_Bit_map[ _Priority_Bits_index(major) ];

  mask = _Priority_Mask( major );
  the_priority_map->ready_major = mask;
  the_priority_map->block_major = ~mask;

  mask = _Priority_Mask( minor );
  the_priority_map->ready_minor = mask;
  the_priority_map->block_minor = ~mask;
}

/*PAGE
 *
 *  _Priority_Add_to_bit_map
 *
 *  DESCRIPTION:
 *
 *  This routine uses the_priority_map to update the priority
 *  bit maps to indicate that a thread has been readied.
 */

RTEMS_INLINE_ROUTINE void _Priority_Add_to_bit_map (
  Priority_Information *the_priority_map
)
{
  *the_priority_map->minor |= the_priority_map->ready_minor;
  _Priority_Major_bit_map  |= the_priority_map->ready_major;
}

/*PAGE
 *
 *  _Priority_Remove_from_bit_map
 *
 *  DESCRIPTION:
 *
 *  This routine uses the_priority_map to update the priority
 *  bit maps to indicate that a thread has been removed from the
 *  ready state.
 */

RTEMS_INLINE_ROUTINE void _Priority_Remove_from_bit_map (
  Priority_Information *the_priority_map
)
{
  *the_priority_map->minor &= the_priority_map->block_minor;
  if ( *the_priority_map->minor == 0 )
    _Priority_Major_bit_map &= the_priority_map->block_major;
}

/*PAGE
 *
 *  _Priority_Get_highest
 *
 *  DESCRIPTION:
 *
 *  This function returns the priority of the highest priority
 *  ready thread.
 */

RTEMS_INLINE_ROUTINE Priority_Control _Priority_Get_highest( void )
{
  Priority_Bit_map_control minor;
  Priority_Bit_map_control major;

  major = cntlzw( _Priority_Major_bit_map );
  minor = cntlzw( _Priority_Bit_map[major] ); 

  return (_Priority_Bits_index( major ) << 4) +
          _Priority_Bits_index( minor );
}

#endif
