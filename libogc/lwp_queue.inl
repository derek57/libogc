#ifndef __LWP_QUEUE_INL__
#define __LWP_QUEUE_INL__

static __inline__ lwp_node* _Chain_Head(lwp_queue *queue)
{
	return (lwp_node*)queue;
}

static __inline__ lwp_node* _Chain_Tail(lwp_queue *queue)
{
	return (lwp_node*)&queue->perm_null;
}

static __inline__ u32 _Chain_Is_tail(lwp_queue *queue,lwp_node *node)
{
	return (node==_Chain_Tail(queue));
}

static __inline__ u32 _Chain_Is_head(lwp_queue *queue,lwp_node *node)
{
	return (node==_Chain_Head(queue));
}

static __inline__ lwp_node* _Chain_Get_first_unprotected(lwp_queue *queue)
{
	lwp_node *ret;
	lwp_node *new_first;
#ifdef _LWPQ_DEBUG
	printk("_Chain_Get_first_unprotected(%p)\n",queue);
#endif

	ret = queue->first;
	new_first = ret->next;
	queue->first = new_first;
	new_first->prev = _Chain_Head(queue);
	return ret;
}

static __inline__ void _Chain_Initialize_empty(lwp_queue *queue)
{
	queue->first = _Chain_Tail(queue);
	queue->perm_null = NULL;
	queue->last = _Chain_Head(queue);
}

static __inline__ u32 _Chain_Is_empty(lwp_queue *queue)
{
	return (queue->first==_Chain_Tail(queue));
}

static __inline__ u32 _Chain_Has_only_one_node(lwp_queue *queue)
{
	return (queue->first==queue->last);
}

static __inline__ void _Chain_Append_unprotected(lwp_queue *queue,lwp_node *node)
{
	lwp_node *old;
#ifdef _LWPQ_DEBUG
	printk("_Chain_Append_unprotected(%p,%p)\n",queue,node);
#endif
	node->next = _Chain_Tail(queue);
	old = queue->last;
	queue->last = node;
	old->next = node;
	node->prev = old;
}

static __inline__ void _Chain_Extract_unprotected(lwp_node *node)
{
#ifdef _LWPQ_DEBUG
	printk("_Chain_Extract_unprotected(%p)\n",node);
#endif
	lwp_node *prev,*next;
	next = node->next;
	prev = node->prev;
	next->prev = prev;
	prev->next = next;
}

static __inline__ void _Chain_Insert_unprotected(lwp_node *after,lwp_node *node)
{
	lwp_node *before;
	
#ifdef _LWPQ_DEBUG
	printk("_Chain_Insert_unprotected(%p,%p)\n",after,node);
#endif
	node->prev = after;
	before = after->next;
	after->next = node;
	node->next = before;
	before->prev = node;
}

static __inline__ void _Chain_Prepend(lwp_queue *queue,lwp_node *node)
{
	_Chain_Insert(_Chain_Head(queue),node);
}

static __inline__ void _Chain_Prepend_unprotected(lwp_queue *queue,lwp_node *node)
{
	_Chain_Insert_unprotected(_Chain_Head(queue),node);
}

static __inline__ lwp_node* _Chain_Get_unprotected(lwp_queue *queue)
{
	if(!_Chain_Is_empty(queue))
		return _Chain_Get_first_unprotected(queue);
	else
		return NULL;
}

#endif
