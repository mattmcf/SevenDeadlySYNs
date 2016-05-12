#include <stdio.h>
#include "Queue.h"

int main()
{
	Queue* queue = queue_new();
	
	const char a[] = "a";
	const char b[] = "b";
	const char c[] = "c";
	const char d[] = "d";
	const char e[] = "e";
	const char f[] = "f";
	
	queue_push(queue, (void*)a);
	queue_push(queue, (void*)b);
	queue_push(queue, (void*)c);
	queue_push(queue, (void*)d);
	queue_push(queue, (void*)e);
	queue_push(queue, (void*)f);
	
	queue_remove(queue, NULL, (void*)d);
	queue_remove(queue, NULL, (void*)e);
	queue_remove(queue, NULL, (void*)c);
	queue_remove(queue, NULL, (void*)f);
	queue_remove(queue, NULL, (void*)a);
	
	for (int i = 0; i < queue_length(queue); i++)
	{
		printf("%s\n", (char*)queue_get(queue, i));
	}
	
	queue_destroy(queue);
	
	return 0;
}

