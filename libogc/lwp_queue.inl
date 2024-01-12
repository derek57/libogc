#ifndef __LWP_QUEUE_INL__
#define __LWP_QUEUE_INL__

static __inline__ Chain_Node* _Chain_Head(Chain_Control *the_chain)
{
	return (Chain_Node*)the_chain;
}

static __inline__ Chain_Node* _Chain_Tail(Chain_Control *the_chain)
{
	return (Chain_Node*)&the_chain->permanent_null;
}

static __inline__ u32 _Chain_Is_tail(Chain_Control *the_chain,Chain_Node *the_node)
{
	return (the_node==_Chain_Tail(the_chain));
}

static __inline__ u32 _Chain_Is_head(Chain_Control *the_chain,Chain_Node *the_node)
{
	return (the_node==_Chain_Head(the_chain));
}

static __inline__ Chain_Node* _Chain_Get_first_unprotected(Chain_Control *the_chain)
{
	Chain_Node *return_node;
	Chain_Node *new_first;
#ifdef _LWPQ_DEBUG
	printk("_Chain_Get_first_unprotected(%p)\n",the_chain);
#endif

	return_node = the_chain->first;
	new_first = return_node->next;
	the_chain->first = new_first;
	new_first->previous = _Chain_Head(the_chain);
	return return_node;
}

static __inline__ void _Chain_Initialize_empty(Chain_Control *the_chain)
{
	the_chain->first = _Chain_Tail(the_chain);
	the_chain->permanent_null = NULL;
	the_chain->last = _Chain_Head(the_chain);
}

static __inline__ u32 _Chain_Is_empty(Chain_Control *the_chain)
{
	return (the_chain->first==_Chain_Tail(the_chain));
}

static __inline__ u32 _Chain_Has_only_one_node(Chain_Control *the_chain)
{
	return (the_chain->first==the_chain->last);
}

static __inline__ void _Chain_Append_unprotected(Chain_Control *the_chain,Chain_Node *the_node)
{
	Chain_Node *old_last_node;
#ifdef _LWPQ_DEBUG
	printk("_Chain_Append_unprotected(%p,%p)\n",the_chain,the_node);
#endif
	the_node->next = _Chain_Tail(the_chain);
	old_last_node = the_chain->last;
	the_chain->last = the_node;
	old_last_node->next = the_node;
	the_node->previous = old_last_node;
}

static __inline__ void _Chain_Extract_unprotected(Chain_Node *the_node)
{
#ifdef _LWPQ_DEBUG
	printk("_Chain_Extract_unprotected(%p)\n",the_node);
#endif
	Chain_Node *previous,*next;
	next = the_node->next;
	previous = the_node->previous;
	next->previous = previous;
	previous->next = next;
}

static __inline__ void _Chain_Insert_unprotected(Chain_Node *after,Chain_Node *the_node)
{
	Chain_Node *before_node;
	
#ifdef _LWPQ_DEBUG
	printk("_Chain_Insert_unprotected(%p,%p)\n",after,the_node);
#endif
	the_node->previous = after;
	before_node = after->next;
	after->next = the_node;
	the_node->next = before_node;
	before_node->previous = the_node;
}

static __inline__ void _Chain_Prepend(Chain_Control *the_chain,Chain_Node *the_node)
{
	_Chain_Insert(_Chain_Head(the_chain),the_node);
}

static __inline__ void _Chain_Prepend_unprotected(Chain_Control *the_chain,Chain_Node *the_node)
{
	_Chain_Insert_unprotected(_Chain_Head(the_chain),the_node);
}

static __inline__ Chain_Node* _Chain_Get_unprotected(Chain_Control *the_chain)
{
	if(!_Chain_Is_empty(the_chain))
		return _Chain_Get_first_unprotected(the_chain);
	else
		return NULL;
}

#endif
