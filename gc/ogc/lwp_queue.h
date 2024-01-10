#ifndef __LWP_QUEUE_H__
#define __LWP_QUEUE_H__

#include <gctypes.h>

//#define _LWPQ_DEBUG

#ifdef _LWPQ_DEBUG
extern int printk(const char *fmt,...);
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Chain_Node_struct Chain_Node; 

struct Chain_Node_struct {
	Chain_Node *next;
	Chain_Node *prev;
};

typedef struct {
	Chain_Node *first;
	Chain_Node *perm_null;
	Chain_Node *last;
} Chain_Control;

void _Chain_Initialize(Chain_Control *,void *,u32,u32);
Chain_Node* _Chain_Get(Chain_Control *);
void _Chain_Append(Chain_Control *,Chain_Node *);
void _Chain_Extract(Chain_Node *);
void _Chain_Insert(Chain_Node *,Chain_Node *);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_queue.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
