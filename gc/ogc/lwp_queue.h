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

typedef struct _lwpnode {
	struct _lwpnode *next;
	struct _lwpnode *prev;
} lwp_node;

typedef struct _lwpqueue {
	lwp_node *first;
	lwp_node *perm_null;
	lwp_node *last;
} lwp_queue;

void _Chain_Initialize(lwp_queue *,void *,u32,u32);
lwp_node* _Chain_Get(lwp_queue *);
void _Chain_Append(lwp_queue *,lwp_node *);
void _Chain_Extract(lwp_node *);
void _Chain_Insert(lwp_node *,lwp_node *);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_queue.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
