#include "AsyncQueue.h"
#include <stdio.h>

int main()
{
	AsyncQueue* queue = asyncqueue_new();
	
	const char a[] = "a";
	const char b[] = "b";
	const char c[] = "c";
	const char d[] = "d";
	const char e[] = "e";
	const char f[] = "f";
	
	for (int j = 0; j < 2000; j++)
	{
		printf("begin %d\n", asyncqueue_length(queue));
		asyncqueue_push(queue, (void*)a);
		asyncqueue_push(queue, (void*)b);
		asyncqueue_push(queue, (void*)c);
		asyncqueue_push(queue, (void*)d);
		asyncqueue_push(queue, (void*)e);
		asyncqueue_push(queue, (void*)f);
	
		for (int i = 0; i < 3; i++)
		{
			printf("%s\n", (char*)asyncqueue_pop(queue));
		}
	}
	
	asyncqueue_destroy(queue);
	
	return 0;
}

