#include "LinkedList.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct _LinkedNode
{
	struct _LinkedNode* forward;
	struct _LinkedNode* back;
	
	void* element;
} _LinkedNode;

typedef struct
{
	_LinkedNode* root;
	_LinkedNode* tail;
	
	int size;
} _LinkedList;

int default_search(void* element, void* key)
{
	return element == key;
}

_LinkedNode* linkednode_new(_LinkedNode* forward, _LinkedNode* back, void* element)
{
	_LinkedNode* ln = (_LinkedNode*)malloc(sizeof(_LinkedNode));
	
	ln->forward = forward;
	ln->back = back;
	ln->element = element;
	
	return ln;
}

void linkednode_destroy(_LinkedNode* node)
{
	free(node);
}

_LinkedNode* linkednode_search(_LinkedNode* n, LinkedListSearchFunction f, void* key)
{
	if (f(n->element, key))
	{
		return n;
	}
	if (n->forward)
	{
		return linkednode_search(n->forward, f, key);
	}
	return NULL;
}

_LinkedNode* linkednode_unlink(_LinkedNode* n)
{
	if (n)
	{
		if (!n->back && !n->forward) 
		{
		}
		else if (n->back && n->forward)
		{
			n->back->forward = n->forward;
			n->forward->back = n->back;
		}
		else if (!n->back)
		{
			n->forward->back = NULL;
		}
		else if (!n->forward)
		{
			n->back->forward = NULL;
		}
	}
	
	return n;
}

_LinkedNode* linkednode_remove(_LinkedNode* root, LinkedListSearchFunction f, void* element)
{
	_LinkedNode* n = linkednode_search(root, f, element);
	
	linkednode_unlink(n);
		
	return n;
}

// ***********************************************************************************
 
LinkedList* linkedlist_new()
{
	_LinkedList* ll = (_LinkedList*)malloc(sizeof(_LinkedList));
	
	ll->root = NULL;
	ll->size = 0;
	
	return (LinkedList*)ll;
}

void linkedlist_destroy(LinkedList* list)
{
	_LinkedList* ll = (_LinkedList*)list;
	
	if (ll->root)
	{
		_LinkedNode* ln = ll->root;
		
		while (ln)
		{
			_LinkedNode* next = ln->forward;
			linkednode_destroy(ln);
			ln = next;
		}
	}
	
	free(ll);
}

void linkedlist_append(LinkedList* list, void* element)
{
	_LinkedList* ll = (_LinkedList*)list;
	
	if (ll->root == NULL)
	{
		ll->root = linkednode_new(NULL, NULL, element);
		ll->tail = ll->root;
		ll->size++;
	}
	else
	{
		_LinkedNode* ln = linkednode_new(NULL, ll->tail, element);
		ll->tail->forward = ln;
		ll->tail = ln;
		ll->size++;
	}
}

void* linkedlist_remove(LinkedList* list, LinkedListSearchFunction f, void* element)
{
	_LinkedList* ll = (_LinkedList*)list;
	if (ll->root == NULL)
	{
		return NULL;
	}
	
	if (!f)
	{
		f = default_search;
	}
	
	_LinkedNode* ln = linkednode_remove(ll->root, f, element);
	
	if (ln == NULL)
	{
		return NULL;
	}
	
	if (ln->back == NULL)
	{
		ll->root = ln->forward;
	}
	if (ln->forward == NULL)
	{
		ll->tail = ln->back;
	}
	
	void* e = ln->element;
	
	linkednode_destroy(ln);
	ll->size--;
		
	return e;
}

void* linkedlist_get(LinkedList* list, LinkedListSearchFunction f, void* element)
{
	_LinkedList* ll = (_LinkedList*)list;
	if (ll->root == NULL)
	{
		return NULL;
	}
	
	if (!f)
	{
		f = default_search;
	}
	
	_LinkedNode* ln = linkednode_search(ll->root, f, element);
	if (ln == NULL)
	{
		return NULL;
	}
	
	return ln->element;
}

int linkedlist_length(LinkedList* list)
{
	_LinkedList* ll = (_LinkedList*)list;
	
	return ll->size;
}

void linkedlist_apply(LinkedList* list, LinkedListApplyFunction func)
{
	_LinkedList* ll = (_LinkedList*)list;
	
	_LinkedNode* ln = ll->root;
	
	while (ln)
	{
		func(ln->element);
		ln = ln->forward;
	}
}

void linkedlist_remove_all(LinkedList* list)
{
	_LinkedList* ll = (_LinkedList*)list;
	
	_LinkedNode* ln = ll->root;
	
	while (ln)
	{
		_LinkedNode* next = ln->forward;
		linkednode_destroy(ln);
		ln = next;
	}
	
	ll->root = NULL;
	ll->tail = NULL;
	ll->size = 0;
}

//****************************** Iterator *********************

typedef struct
{
	_LinkedList* list;
	_LinkedNode* ln;
	int index;
} _LinkedListIterator;

LinkedListIterator* linkedlistiterator_new(LinkedList* list)
{
	_LinkedList* ll = (_LinkedList*)list;
	
	_LinkedListIterator* lli = (_LinkedListIterator*)malloc(sizeof(_LinkedListIterator));
	lli->ln = NULL;
	lli->list = ll;
	lli->index = 0;
	
	return (LinkedListIterator*)(lli);
}

void* linkedlistiterator_next(LinkedListIterator* iterator)
{	
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	if (lli->ln == NULL && lli->list->root != NULL)
	{
		lli->ln = lli->list->root;
		lli->index = 0;
		return lli->ln->element;
	}
	
	if (lli->ln != NULL && lli->ln->forward != NULL)
	{
		lli->ln = lli->ln->forward;
		lli->index++;
		return lli->ln->element;
	}	
		
	return NULL;
}

void* linkedlistiterator_current(LinkedListIterator* iterator)
{
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	if (lli->ln != NULL)
	{
		return lli->ln->element;
	}
	
	return NULL;
}

void* linkedlistiterator_previous(LinkedListIterator* iterator)
{
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	if (lli->ln == NULL && lli->list->tail != NULL)
	{
		lli->ln = lli->list->tail;
		lli->index = lli->list->size - 1;
		return lli->ln->element;
	}
	
	if (lli->ln->back != NULL)
	{
		lli->ln = lli->ln->back;	
		lli->index--;	
		return lli->ln->element;
	}
	
	return NULL;
}

int linkedlistiterator_insert_before(LinkedListIterator* iterator, void* element)
{
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	if (lli->ln != NULL)
	{
		_LinkedNode* newNode = NULL;
		if (lli->ln->back == NULL)
		{
			newNode = linkednode_new(lli->ln, NULL, element);
			lli->ln->back = newNode;
			lli->list->root = newNode;
		}
		else
		{
			newNode = linkednode_new(lli->ln, lli->ln->back, element);
			lli->ln->back->forward = newNode;
			lli->ln->back = newNode;
		}
		lli->index++;
		lli->list->size++;
		return 1;
	}
	
	return 0;
}

int linkedlistiterator_insert_after(LinkedListIterator* iterator, void* element)
{
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	if (lli->ln != NULL)
	{
		_LinkedNode* newNode = NULL;
		if (lli->ln->forward == NULL)
		{
			newNode = linkednode_new(NULL, lli->ln, element);
			lli->ln->forward = newNode;
			lli->list->tail = newNode;
		}
		else
		{
			newNode = linkednode_new(lli->ln->forward, lli->ln, element);
			lli->ln->forward->back = newNode;
			lli->ln->forward = newNode;
		}
		lli->list->size++;
		return 1;
	}
	
	return 0;
}

void* linkedlistiterator_remove(LinkedListIterator* iterator)
{
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	if (lli->ln != NULL)
	{
		_LinkedNode* toRemove = lli->ln;
		lli->ln = toRemove->back;
		linkednode_unlink(toRemove);
		
		if (toRemove->back == NULL)
		{
			lli->list->root = toRemove->forward;
		}
		if (toRemove->forward == NULL)
		{
			lli->list->tail = toRemove->back;
		}
		
		void* data = toRemove->element;
		linkednode_destroy(toRemove);
		lli->list->size--;
		lli->index--;
		
		return data;
	}
	
	return NULL;
}

int linkedlistiterator_index(LinkedListIterator* iterator)
{
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	return lli->index;
}

void linkedlistiterator_reset(LinkedListIterator* iterator)
{		
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	lli->ln = NULL;
}

void linkedlistiterator_destroy(LinkedListIterator* iterator)
{
	_LinkedListIterator* lli = (_LinkedListIterator*)iterator;
	
	free(lli);
}




