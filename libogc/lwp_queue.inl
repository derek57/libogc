#ifndef __LWP_QUEUE_INL__
#define __LWP_QUEUE_INL__

static __inline__ Chain_Node* _Chain_Head(Chain_Control *queue)
{
	return (Chain_Node*)queue;
}

static __inline__ Chain_Node* _Chain_Tail(Chain_Control *queue)
{
	return (Chain_Node*)&queue->permanent_null;
}

static __inline__ u32 _Chain_Is_tail(Chain_Control *queue,Chain_Node *node)
{
	return (node==_Chain_Tail(queue));
}

static __inline__ u32 _Chain_Is_head(Chain_Control *queue,Chain_Node *node)
{
	return (node==_Chain_Head(queue));
}

static __inline__ Chain_Node* _Chain_Get_first_unprotected(Chain_Control *queue)
{
	Chain_Node *ret;
	Chain_Node *new_first;
#ifdef _LWPQ_DEBUG
	printk("_Chain_Get_first_unprotected(%p)\n",queue);
#endif

	ret = queue->first;
	new_first = ret->next;
	queue->first = new_first;
	new_first->previous = _Chain_Head(queue);
	return ret;
}

static __inline__ void _Chain_Initialize_empty(Chain_Control *queue)
{
	queue->first = _Chain_Tail(queue);
	queue->permanent_null = NULL;
	queue->last = _Chain_Head(queue);
}

static __inline__ u32 _Chain_Is_empty(Chain_Control *queue)
{
	return (queue->first==_Chain_Tail(queue));
}

static __inline__ u32 _Chain_Has_only_one_node(Chain_Control *queue)
{
	return (queue->first==queue->last);
}

static __inline__ void _Chain_Append_unprotected(Chain_Control *queue,Chain_Node *node)
{
	Chain_Node *old;
#ifdef _LWPQ_DEBUG
	printk("_Chain_Append_unprotected(%p,%p)\n",queue,node);
#endif
	node->next = _Chain_Tail(queue);
	old = queue->last;
	queue->last = node;
	old->next = node;
	node->previous = old;
}

static __inline__ void _Chain_Extract_unprotected(Chain_Node *node)
{
#ifdef _LWPQ_DEBUG
	printk("_Chain_Extract_unprotected(%p)\n",node);
#endif
	Chain_Node *prev,*next;
	next = node->next;
	prev = node->previous;
	next->previous = prev;
	prev->next = next;
}

static __inline__ void _Chain_Insert_unprotected(Chain_Node *after,Chain_Node *node)
{
	Chain_Node *before;
	
#ifdef _LWPQ_DEBUG
	printk("_Chain_Insert_unprotected(%p,%p)\n",after,node);
#endif
	node->previous = after;
	before = after->next;
	after->next = node;
	node->next = before;
	before->previous = node;
}

static __inline__ void _Chain_Prepend(Chain_Control *queue,Chain_Node *node)
{
	_Chain_Insert(_Chain_Head(queue),node);
}

static __inline__ void _Chain_Prepend_unprotected(Chain_Control *queue,Chain_Node *node)
{
	_Chain_Insert_unprotected(_Chain_Head(queue),node);
}

static __inline__ Chain_Node* _Chain_Get_unprotected(Chain_Control *queue)
{
	if(!_Chain_Is_empty(queue))
		return _Chain_Get_first_unprotected(queue);
	else
		return NULL;
}

#endif
