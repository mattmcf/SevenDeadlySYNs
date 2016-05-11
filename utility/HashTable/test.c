#include <stdio.h>
#include "HashTable.h"

int main()
{
	HashTable* ht = hashtable_new(NULL);
	
	const char a[] = "a";
	const char b[] = "b";
	const char c[] = "c";
	const char d[] = "d";
	const char e[] = "e";
	const char f[] = "f";
	
	hashtable_add(ht, (void*)a);
	hashtable_add(ht, (void*)b);
	hashtable_add(ht, (void*)c);
	hashtable_add(ht, (void*)d);
	hashtable_add(ht, (void*)e);
	hashtable_add(ht, (void*)f);
		
	HashTableIterator* hti = hashtableiterator_new(ht);
	char* element;
	while ((element = hashtableiterator_next(hti)))
	{
		printf("%s\n", element);
	}
	hashtableiterator_destroy(hti);
	
	printf("_______\n");
	
	printf("%s\n", (char*)hashtable_remove_element(ht, (void*)e));
	printf("%s\n", (char*)hashtable_remove_element(ht, (void*)b));
	printf("%s\n", (char*)hashtable_remove_element(ht, (void*)a));
	printf("%s\n", (char*)hashtable_remove_element(ht, (void*)f));
	printf("%s\n", (char*)hashtable_remove_element(ht, (void*)c));
	printf("%s\n", (char*)hashtable_remove_element(ht, (void*)d));
	
	hashtable_destroy(ht);
	
	return 0;
}

