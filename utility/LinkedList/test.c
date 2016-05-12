#include <stdio.h>
#include "LinkedList.h"

int main()
{
	LinkedList* ll = linkedlist_new();
	
	const char a[] = "a";
	const char b[] = "b";
	const char c[] = "c";
	const char d[] = "d";
	const char e[] = "e";
	const char f[] = "f";
	
	linkedlist_append(ll, (void*)a);
	linkedlist_append(ll, (void*)b);
	linkedlist_append(ll, (void*)c);
	linkedlist_append(ll, (void*)d);
	linkedlist_append(ll, (void*)e);
	linkedlist_append(ll, (void*)f);
		
	LinkedListIterator* lli = linkedlistiterator_new(ll);
	void* element;
	while ((element = linkedlistiterator_next(lli)))
	{
		printf("%s\n", (char*)element);
		if (element != a)
		{
			linkedlistiterator_remove(lli);
		}
	}	
	
	linkedlistiterator_remove(lli);
	
	printf("-------------\n");
	
	linkedlistiterator_reset(lli);
	
	while ((element = linkedlistiterator_next(lli)))
	{
		printf("%s\n", (char*)element);
	}	
	
	
	printf("-------------\n");
	
	printf("%s\n", (char*)linkedlist_remove(ll, NULL, (void*)e));
	printf("%s\n", (char*)linkedlist_remove(ll, NULL, (void*)b));
	printf("%s\n", (char*)linkedlist_remove(ll, NULL, (void*)a));
	printf("%s\n", (char*)linkedlist_remove(ll, NULL, (void*)f));
	printf("%s\n", (char*)linkedlist_remove(ll, NULL, (void*)c));
	printf("%s\n", (char*)linkedlist_remove(ll, NULL, (void*)d));
		
	linkedlist_destroy(ll);
	
	return 0;
}

