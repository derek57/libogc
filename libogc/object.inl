/*  object.inl
 *
 *  This include file contains the RTEMS_INLINE_ROUTINE implementation of all
 *  of the inlined routines in the Object Handler.
 *
 *  COPYRIGHT (c) 1989-2002.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 */

#ifndef __OBJECTS_inl
#define __OBJECTS_inl

/**
 *  This function sets the pointer to the local_table object
 *  referenced by the index.
 *
 *  @param[in] information points to an Object Information Table
 *  @param[in] index is the index of the object the caller wants to access
 *  @param[in] the_object is the local object pointer
 */

RTEMS_INLINE_ROUTINE void _Objects_Set_local_object(
  Objects_Information *information,
  unsigned32           index,
  Objects_Control     *the_object
)
{
  if ( index < information->maximum )
    information->local_table[ index ] = the_object;
}

/*PAGE
 *
 *  _Objects_Open
 *
 *  DESCRIPTION:
 *
 *  This function places the_object control pointer and object name
 *  in the Local Pointer and Local Name Tables, respectively.
 */

RTEMS_INLINE_ROUTINE void _Objects_Open(
  Objects_Information *information,
  Objects_Control     *the_object
)
{
  _Objects_Set_local_object( information, the_object->id, the_object );
}

/*PAGE
 *
 *  _Objects_Close
 *
 *  DESCRIPTION:
 *
 *  This function removes the_object control pointer and object name
 *  in the Local Pointer and Local Name Tables.
 */

RTEMS_INLINE_ROUTINE void _Objects_Close(
  Objects_Information  *information,
  Objects_Control      *the_object
)
{
  _Objects_Set_local_object( information, the_object->id, NULL );
}

#endif
