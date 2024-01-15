/*
 *  Object Handler
 *
 *
 *  COPYRIGHT (c) 1989, 1990, 1991, 1992, 1993, 1994.
 *  On-Line Applications Research Corporation (OAR).
 *  All rights assigned to U.S. Government, 1994.
 *
 *  This material may be reproduced by or for the U.S. Government pursuant
 *  to the copyright license under the clause at DFARS 252.227-7013.  This
 *  notice must appear in all copies of this file and its derivatives.
 *
 *  $Id$
 */

#include <processor.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lwp_threads.h>
#include <lwp_wkspace.h>
#include <lwp_config.h>

#include "lwp_objmgr.h"

static unsigned32 _lwp_objmgr_memsize = 0;
static Objects_Control *null_local_table = NULL;

unsigned32 __lwp_objmgr_memsize(void)
{
	return _lwp_objmgr_memsize;
}

/*PAGE
 *
 *  _Objects_Initialize_information
 *
 *  This routine initializes all object information related data structures.
 *
 *  Input parameters:
 *    information     - object information table
 *    maximum         - maximum objects of this class
 *    size            - size of this object's control block
 *
 *  Output parameters:  NONE
 */

void _Objects_Initialize_information(
  Objects_Information *information,
  unsigned32           maximum,
  unsigned32           size
)
{
  unsigned32       index;
  unsigned32       i;
  unsigned32       sz;
  Objects_Control *the_object;
  Chain_Control    inactives;
  void            **local_table;

  /*
   *  Calculate minimum and maximum Id's
   */

  information->minimum_id    = 0;
  information->maximum_id    = 0;

  information->inactive      = 0;

  /*
   *  Set the size of the object
   */

  information->size          = size;

  information->maximum       = maximum;
  information->object_blocks = NULL;

  /*
   *  Provide a null local table entry for the case of any empty table.
   */

  information->local_table   = &null_local_table;

  _Chain_Initialize_empty( &information->Inactive );

  sz = ( ( information->maximum * sizeof(Objects_Control *) ) + ( information->maximum * information->size ) );
  local_table = (void **)_Workspace_Allocate( information->maximum * sizeof(Objects_Control *) );

  if ( !local_table )
    return;

  /*
   *  Allocate local pointer table
   */

  information->local_table = (Objects_Control **)local_table;

  /*
   *  Initialize local pointer table
   */

  for ( i=0; i < information->maximum; i++ ) {
    local_table[ i ] = NULL;
  }

  information->object_blocks = _Workspace_Allocate( information->maximum * information->size );

  if ( !information->object_blocks ) {
    _Workspace_Free( local_table );
    return;
  }

  /*
   *  Initialize objects .. if there are any
   */

  _Chain_Initialize( &inactives, information->object_blocks, information->maximum, information->size );

  index = information->minimum_id;

  while ( ( the_object = (Objects_Control *)_Chain_Get( &inactives ) ) != NULL ) {
    the_object->id = index;
    the_object->information = NULL;
    _Chain_Append( &information->Inactive, &the_object->Node );
    index++;
  }

  information->maximum_id += information->maximum;
  information->inactive   += information->maximum;
  _lwp_objmgr_memsize     += sz;
}

/*PAGE
 *
 * _Objects_Get_isr_disable
 *
 * This routine sets the object pointer for the given
 * object id based on the given object information structure.
 *
 * Input parameters:
 *   information - pointer to entry in table for this class
 *   id          - object id to search for
 *   level       - pointer to previous interrupt level
 *
 * Output parameters:
 *   returns  - address of object if local
 *  *level    - previous interrupt level
 */

Objects_Control *_Objects_Get_isr_disable(
  Objects_Information *information,
  Objects_Id           id,
  ISR_Level           *level_p
)
{
  ISR_Level        level;
  Objects_Control *the_object = NULL;

  _ISR_Disable(level);
  if ( information->maximum_id >= id) {
    if ( (the_object = information->local_table[ id ]) != NULL ) {
      *level_p = level;
      return the_object;
    }
  }
  _ISR_Enable(level);

  return NULL;
}

/*PAGE
 *
 * _Objects_Get_no_protection
 *
 * This routine sets the object pointer for the given
 * object id based on the given object information structure.
 *
 * Input parameters:
 *   information - pointer to entry in table for this class
 *   id          - object id to search for
 *
 * Output parameters:
 *   returns  - address of object if local
 */

Objects_Control *_Objects_Get_no_protection(
  Objects_Information *information,
  Objects_Id           id
)
{
  Objects_Control *the_object = NULL;

  if ( information->maximum_id >= id) {
    if ( (the_object = information->local_table[ id ]) != NULL )
      return the_object;
  }

  return NULL;
}

/*PAGE
 *
 * _Objects_Get
 *
 * This routine sets the object pointer for the given
 * object id based on the given object information structure.
 *
 * Input parameters:
 *   information - pointer to entry in table for this class
 *   id          - object id to search for
 *
 * Output parameters:
 *   returns - address of object if local
 */

Objects_Control *_Objects_Get(
  Objects_Information *information,
  Objects_Id           id
)
{
  Objects_Control *the_object = NULL;

  if ( information->maximum_id >= id ) {
    _Thread_Disable_dispatch();

    if ( (the_object = information->local_table[ id ]) != NULL )
      return the_object;

    _Thread_Enable_dispatch();
  }

  return NULL;
}

/*PAGE
 *
 *  _Objects_Allocate
 *
 *  DESCRIPTION:
 *
 *  This function allocates a object control block from
 *  the inactive chain of free object control blocks.
 */

Objects_Control *_Objects_Allocate(
  Objects_Information *information
)
{
  ISR_Level level;
  Objects_Control *the_object;

  _ISR_Disable(level);
  the_object = (Objects_Control *) _Chain_Get_unprotected( &information->Inactive );

  if ( the_object ) {
    the_object->information = information;
    information->inactive--;
  }

  _ISR_Enable(level);

  return the_object;
}

/*PAGE
 *
 *  _Objects_Free
 *
 *  DESCRIPTION:
 *
 *  This function frees a object control block to the
 *  inactive chain of free object control blocks.
 */

void _Objects_Free(
  Objects_Information *information,
  Objects_Control     *the_object
)
{
	ISR_Level level;

	_ISR_Disable(level);
	_Chain_Append_unprotected( &information->Inactive, &the_object->Node );
	the_object->information	= NULL;
	information->inactive++;
	_ISR_Enable(level);
}
