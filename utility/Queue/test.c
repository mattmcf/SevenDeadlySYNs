#include <stdio.h>
#include "Queue.h"

int filter(void* a, void* b)
{
	printf("%d %d\n", (int)(long)a, (int)(long)b);
	return (int)(long)a < 'c';
}

int main()
{
	Queue* queue = queue_new();
	
	queue_push(queue, (void*)'a');
	queue_push(queue, (void*)'b');
	queue_push(queue, (void*)'c');
	queue_push(queue, (void*)'d');
	queue_push(queue, (void*)'e');
	queue_push(queue, (void*)'f');
	
	Queue* filtered = queue_filter(queue, filter, (void*)1);
	
	/*
	queue_remove(queue, NULL, (void*)d);
	queue_remove(queue, NULL, (void*)e);
	queue_remove(queue, NULL, (void*)c);
	queue_remove(queue, NULL, (void*)f);
	queue_remove(queue, NULL, (void*)a);
	*/
	for (int i = 0; i < queue_length(filtered); i++)
	{
		printf("%c\n", (char)queue_get(filtered, i));
	}
	
	queue_destroy(queue);
	
	return 0;
}

