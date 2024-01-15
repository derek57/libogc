/*  chain.h
 *
 *  This include file contains all the constants and structures associated
 *  with the Doubly Linked Chain Handler.
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

#ifndef __LWP_QUEUE_H__
#define __LWP_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <gctypes.h>

#include "lwp_config.h"

//#define _LWPQ_DEBUG

#ifdef _LWPQ_DEBUG
extern int printk(const char *fmt,...);
#endif

/**
 *  @typedef Chain_Node
 *
 *  This type definition promotes the name for the Chain Node used by
 *  all RTEMS code.  It is a separate type definition because a forward
 *  reference is required to define it.  See @ref Chain_Node_struct for
 *  detailed information.
 */

typedef struct Chain_Node_struct Chain_Node;

/*
 *  This is used to manage each element (node) which is placed
 *  on a chain.
 *
 *  NOTE:  Typically, a more complicated structure will use the
 *         chain package.  The more complicated structure will
 *         include a chain node as the first element in its
 *         control structure.  It will then call the chain package
 *         with a pointer to that node element.  The node pointer
 *         and the higher level structure start at the same address
 *         so the user can cast the pointers back and forth.
 *
 */

struct Chain_Node_struct {
  /** This points to the node after this one on this chain. */
  Chain_Node *next;
  /** This points to the node immediate prior to this one on this chain. */
  Chain_Node *previous;
};

/*
 *  This is used to manage a chain.  A chain consists of a doubly
 *  linked list of zero or more nodes.
 *
 *  NOTE:  This implementation does not require special checks for
 *         manipulating the first and last elements on the chain.
 *         To accomplish this the chain control structure is
 *         treated as two overlapping chain nodes.  The permanent
 *         head of the chain overlays a node structure on the
 *         first and permanent_null fields.  The permanent tail
 *         of the chain overlays a node structure on the
 *         permanent_null and last elements of the structure.
 *
 */

typedef struct {
  /** This points to the first node on this chain. */
  Chain_Node *first;
  /** This field is always 0. */
  Chain_Node *permanent_null;
  /** This points to the last node on this chain. */
  Chain_Node *last;
} Chain_Control;

/*
 *  _Chain_Initialize
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the_chain structure to manage the
 *  contiguous array of number_nodes nodes which starts at
 *  starting_address.  Each node is of node_size bytes.
 *
 */

void _Chain_Initialize(
  Chain_Control *the_chain,
  void          *starting_address,
  size_t         number_nodes,
  size_t         node_size
);

/*
 *  _Chain_Extract
 *
 *  DESCRIPTION:
 *
 *  This routine extracts the_node from the chain on which it resides.
 *  It disables interrupts to insure the atomicity of the
 *  extract operation.
 *
 */

void _Chain_Extract(
  Chain_Node *the_node
);

/*
 *  _Chain_Get
 *
 *  DESCRIPTION:
 *
 *  This function removes the first node from the_chain and returns
 *  a pointer to that node.  If the_chain is empty, then NULL is returned.
 *  It disables interrupts to insure the atomicity of the
 *  get operation.
 *
 */

Chain_Node *_Chain_Get(
  Chain_Control *the_chain
);

/*
 *  _Chain_Insert
 *
 *  DESCRIPTION:
 *
 *  This routine inserts the_node on a chain immediately following
 *  after_node.  It disables interrupts to insure the atomicity
 *  of the extract operation.
 *
 */

void _Chain_Insert(
  Chain_Node *after_node,
  Chain_Node *the_node
);

/*
 *  _Chain_Append
 *
 *  DESCRIPTION:
 *
 *  This routine appends the_node onto the end of the_chain.
 *  It disables interrupts to insure the atomicity of the
 *  append operation.
 *
 */

void _Chain_Append(
  Chain_Control *the_chain,
  Chain_Node    *the_node
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_queue.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
