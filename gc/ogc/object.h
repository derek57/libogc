/*
 *  This include file contains all the constants and structures associated
 *  with the Object Handler.  This Handler provides mechanisms which
 *  can be used to initialize and manipulate all objects which have
 *  ids.
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

#ifndef __OBJECTS_h
#define __OBJECTS_h

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include "lwp_queue.h"

/**
 *  This is the bit position of the starting bit of the index portion of
 *  the object Id.
 */

#define OBJECTS_INDEX_START_BIT     0

/**
 *  This is the bit position of the starting bit of the node portion of
 *  the object Id.
 */

#define OBJECTS_NODE_START_BIT      16

/**
 *  This mask is used to extract the index portion of an object Id.
 */
#define OBJECTS_INDEX_MASK          0x0000ffff

/*PAGE
 *
 *  _Objects_Get_node
 *
 */

#define _Objects_Get_node( _id ) \
  (((_id) >> OBJECTS_NODE_START_BIT) & OBJECTS_INDEX_MASK) 

/*PAGE
 *
 *  _Objects_Get_index
 *
 */

#define _Objects_Get_index( _id ) \
  (((_id) >> OBJECTS_INDEX_START_BIT) & OBJECTS_INDEX_MASK)  

/*PAGE
 *
 *  _Objects_Build_id
 *
 */

#define _Objects_Build_id( _node, _index ) \
  ( ((_node) << OBJECTS_NODE_START_BIT)       | \
    ((_index) << OBJECTS_INDEX_START_BIT) ) 

/*
 *  This enumerated type is used in the class field of the object ID.
 */

typedef enum {
  OBJECTS_NO_CLASS                  =  0,
  OBJECTS_INTERNAL_THREADS          =  1,
  OBJECTS_RTEMS_TASKS               =  2,
  OBJECTS_POSIX_MUTEXES             =  3,
  OBJECTS_POSIX_SEMAPHORES          =  4,
  OBJECTS_POSIX_CONDITION_VARIABLES =  5,
  OBJECTS_POSIX_MESSAGE_QUEUES      =  6,
  OBJECTS_RTEMS_PORTS               =  7
} Objects_Classes;

/**
 *  The following defines the structure for the information used to
 *  manage each class of objects.
 */

typedef struct _Objects_Control Objects_Information;

/**
 *  The following defines the Object Control Block used to manage
 *  each object local to this node.
 */

typedef struct {
  /** This is the chain node portion of an object. */
  Chain_Node           Node;
  /** This is the object's ID. */
  signed32             id;
  /** This is the object's information. */
  Objects_Information *information;
} Objects_Control;

struct _Objects_Control {
  /** This is the minimum valid id of this object class. */
  Objects_Id        minimum_id;
  /** This is the maximum valid id of this object class. */
  Objects_Id        maximum_id;
  /** This is the maximum number of objects in this class. */
  Objects_Maximum   maximum;
  /** This is the size in bytes of each object instance. */
  unsigned32        size;
  /** This points to the table of local objects. */
  Objects_Control **local_table;
  /** This is a table to the chain of inactive object memory blocks. */
  void            **object_blocks;
  /** This is the chain of inactive control blocks. */
  Chain_Control     Inactive;
  /** This is the number of objects on the Inactive list. */
  Objects_Maximum   inactive;
};

/*
 *  _Objects_Initialize_information
 *
 *  DESCRIPTION:
 *
 *  This function initializes an object class information record.
 *  SUPPORTS_GLOBAL is TRUE if the object class supports global
 *  objects, and FALSE otherwise.  Maximum indicates the number
 *  of objects required in this class and size indicates the size
 *  in bytes of each control block for this object class.  The
 *  name length and string designator are also set.  In addition,
 *  the class may be a task, therefore this information is also included.
 */

void _Objects_Initialize_information (
  Objects_Information *information,
  unsigned32           maximum,
  unsigned32           size
);

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
);

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
);

/*
 *  _Objects_Get
 *
 *  DESCRIPTION:
 *
 *  This function maps object ids to object control blocks.
 *  If id corresponds to a local object, then it returns
 *  the_object control pointer which maps to id and location
 *  is set to OBJECTS_LOCAL.  If the object class supports global
 *  objects and the object id is global and resides on a remote
 *  node, then location is set to OBJECTS_REMOTE, and the_object
 *  is undefined.  Otherwise, location is set to OBJECTS_ERROR
 *  and the_object is undefined.
 *
 *  NOTE: _Objects_Get returns with dispatching disabled for 
 *        local and remote objects.
 *        _Objects_Get_isr_disable returns with dispatching
 *        disabled for remote objects and interrupts for local
 *        objects.
 */

Objects_Control *_Objects_Get (
  Objects_Information *information,
  Objects_Id           id
);

/**
 *  This function maps object ids to object control blocks.
 *  If id corresponds to a local object, then it returns
 *  the_object control pointer which maps to id and location
 *  is set to OBJECTS_LOCAL.  If the object class supports global
 *  objects and the object id is global and resides on a remote
 *  node, then location is set to OBJECTS_REMOTE, and the_object
 *  is undefined.  Otherwise, location is set to OBJECTS_ERROR
 *  and the_object is undefined.
 *
 *  @param[in] information points to an object class information block.
 *  @param[in] id is the Id of the object whose name we are locating.
 *  @param[in] location will contain an indication of success or failure.
 *  @param[in] level is the interrupt level being turned.
 *
 *  @return This method returns one of the values from the 
 *          @ref Objects_Name_or_id_lookup_errors enumeration to indicate
 *          successful or failure.  On success @a name will contain the name of
 *          the requested object.
 *
 *  @note _Objects_Get returns with dispatching disabled for
 *  local and remote objects.  _Objects_Get_isr_disable returns with
 *  dispatching disabled for remote objects and interrupts for local
 *  objects.
 */

Objects_Control *_Objects_Get_isr_disable(
  Objects_Information *information,
  Objects_Id           id,
  ISR_Level           *level
);

/**
 *  This function maps object ids to object control blocks.
 *  If id corresponds to a local object, then it returns
 *  the_object control pointer which maps to id and location
 *  is set to OBJECTS_LOCAL.  If the object class supports global
 *  objects and the object id is global and resides on a remote
 *  node, then location is set to OBJECTS_REMOTE, and the_object
 *  is undefined.  Otherwise, location is set to OBJECTS_ERROR
 *  and the_object is undefined.
 *
 *  @param[in] information points to an object class information block.
 *  @param[in] id is the Id of the object whose name we are locating.
 *  @param[in] location will contain an indication of success or failure.
 *
 *  @return This method returns one of the values from the 
 *          @ref Objects_Name_or_id_lookup_errors enumeration to indicate
 *          successful or failure.  On success @a id will contain the Id of
 *          the requested object.
 *
 *  @note _Objects_Get returns with dispatching disabled for
 *  local and remote objects.  _Objects_Get_isr_disable returns with
 *  dispatching disabled for remote objects and interrupts for local
 *  objects.
 */

Objects_Control *_Objects_Get_no_protection(
  Objects_Information *information,
  Objects_Id           id
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/object.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
