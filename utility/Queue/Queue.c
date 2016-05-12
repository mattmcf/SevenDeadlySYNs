
/*
 * queue.c -- Implentation of queue.h
 *   Written by Jacob Weiss
 */

#include "Queue.h"
#include <stdlib.h>
#include <stdio.h>
	 
#define INITIAL_QUEUE_SPACE 10

typedef struct 
{
	void** elements;
	int length;
	int space;
	int head; // Push to head
	int tail; // Pop from tail
} _Queue;

Queue* queue_new()    
{
	_Queue* q = (_Queue*)malloc(sizeof(_Queue));

	q->elements = (void**)malloc(INITIAL_QUEUE_SPACE * sizeof(void*));
	q->length = 0;
	q->head = 0;
	q->tail = 0;
	q->space = INITIAL_QUEUE_SPACE;
		
	return (Queue*)q;
}

int default_search_function(void* elementp, void* keyp)
{
	return elementp == keyp;
}

void queue_destroy(Queue* qp)
{
	_Queue* q = (_Queue*)qp;

	free(q->elements);
	free(q);
}

int queue_length(Queue* qp)
{
	_Queue* q = (_Queue*)qp;
	
	return q->length;
}

void queue_shiftleft(Queue* qp, int index)
{
	_Queue* q = (_Queue*)qp;

	for (int i = index; i < q->length; i++)
	{	
		q->elements[(i - 1 + q->tail + q->space) % q->space] = q->elements[(i + q->tail) % q->space];
	}
}

void queue_shiftright(Queue* qp, int index)
{
	_Queue* q = (_Queue*)qp;

	for (int i = index; i >=0; i--)
	{	
		q->elements[(i + 1 + q->tail) % q->space] = q->elements[(i + q->tail) % q->space];
	}
}


void queue_push(Queue* qp, void* elementp)
{
	_Queue* q = (_Queue*)qp;

	if (q->length == q->space)
	{
		q->space *= 2;
		void** newElements = (void**)malloc(q->space * sizeof(void*));
		
		for (int i = 0; i < q->length; i++)
		{
			newElements[i] = q->elements[(i + q->tail) % q->length];
		}
		
		free(q->elements);
		q->elements = newElements;
		
		q->tail = 0;
		q->head = q->length;
	}

	q->elements[q->head] = elementp;
	q->head = (q->head + 1) % q->space;
	q->length++;
}

void* queue_spop(Queue* qu)
{
	_Queue* q = (_Queue*)qu;

	if (q->length == 0)
	{
		return NULL;
	}
	
	q->length--;
	q->head = (q->head - 1 + q->space) % q->space;
	void* element = q->elements[q->head];
	return element;
}
void* queue_speek(Queue* qp)
{
	void* element = queue_spop(qp);
	if (element)
	{
		queue_push(qp, element);
	}
	return element;
}
void* queue_pop(Queue* qp)
{
	_Queue* q = (_Queue*)qp;

	if (q->length == 0)
	{
		return NULL;
	}

	void* element = q->elements[q->tail];

	q->tail = (q->tail + 1) % q->space;
	q->length--;

	return element;
}


void* queue_get(Queue* qp, int index)
{
	_Queue* q = (_Queue*)qp;
	
	return q->elements[(index + q->tail) % q->space];
}

void queue_apply(Queue* qp, QueueApplyFunction af)
{
	_Queue* q = (_Queue*)qp;

	for (int i = 0; i < q->length; i++)
	{
		af(q->elements[(i + q->tail) % q->space]);
	}
}
Queue* queue_filter(Queue* qp, QueueFilterFunction filter, void* userData)
{
	_Queue* q = (_Queue*)qp;

	Queue* filtered = queue_new();

	for (int i = 0; i < q->length; i++)
	{
		void* element = q->elements[(i + q->tail) % q->space];
		if (filter(element, userData))
		{
			queue_push(filtered, element);
		}
	}
	return filtered;
}
Queue* queue_map(Queue* qp, QueueMapFunction map)
{
	_Queue* q = (_Queue*)qp;

	Queue* mapped = queue_new();

	for (int i = 0; i < q->length; i++)
	{
		queue_push(mapped, map(q->elements[(i + q->tail) % q->space]));
	}
	return mapped;	
}

int qsearchi(Queue* qp, int (*searchfn)(void* elementp, void* keyp), void* skeyp)
{
  _Queue* q = (_Queue*)qp;

	for (int i = 0; i < q->length; i++)
	{
		void* element = q->elements[(i + q->tail) % q->space];
		if (searchfn(skeyp, element))
		{
			return i;
		}
	}
	return -1;
}

void* queue_search(Queue* qp, QueueSearchFunction sf, void* skeyp)
{	
	int index = qsearchi(qp, sf, skeyp);
	
	if (index == -1)
	{
		return NULL;
	}
	
	void* element = queue_get(qp, index);
	
	return element;
}

void* queue_remove(Queue* qp, QueueSearchFunction sf, void* skeyp)
{
	_Queue* q = (_Queue*)qp;

	if (sf == NULL)
	{
		sf = default_search_function;
	}

	int index = qsearchi(qp, sf, skeyp);

	if (index == -1)
	{
		return NULL;
	}

	void* element = queue_get(qp, index);

	if (index > q->length / 2)
	{
		queue_shiftleft(qp, index + 1);
		q->head = ((q->head - 1) + q->space) % q->space;
	}
	else
	{
		queue_shiftright(qp, index - 1);
		q->tail = (q->tail + 1) % q->space;
	}
	q->length--;

	return element;
}

void queue_concat(Queue* q1p, Queue* q2p)
{
	void* element = queue_pop(q2p);

	while (element)
	{
		queue_push(q1p, element);
		element = queue_pop(q2p);
	}
}
