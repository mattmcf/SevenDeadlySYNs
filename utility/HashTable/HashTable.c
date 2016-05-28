#include "HashTable.h"
#include "../LinkedList/LinkedList.h"
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_TABLE_SIZE 1023

typedef struct
{
	LinkedList** table;
	int tablesize;
	HashFunction hash;
	ElementEqualsFunction equals;
} _HashTable;

typedef struct
{
	HashFunction hash;
	ElementEqualsFunction equals;
	void* key;
} _BucketSearch;

typedef struct
{
	int hashValue;
	void* element;
} _HashedElement;

int default_hash_function(void* element)
{
	return (int)(long)element;
}

int bucket_search_function(void* element, _BucketSearch* search)
{
	return (search->hash(element) == search->hash(search->key)) && (search->equals(element, search->key));
}

HashTable* hashtable_new(HashFunction hashfunc, ElementEqualsFunction equals)
{
	_HashTable* ht = (_HashTable*)malloc(sizeof(_HashTable));
	
	ht->table = (LinkedList**)calloc(INITIAL_TABLE_SIZE, sizeof(LinkedList*));
	
	for (int i = 0; i < INITIAL_TABLE_SIZE; i++)
	{
		ht->table[i] = linkedlist_new();
	}
	
	if (hashfunc)
	{
		ht->hash = hashfunc;
	}
	else
	{
		ht->hash = default_hash_function;
	}
	
	ht->equals = equals;
	
	ht->tablesize = INITIAL_TABLE_SIZE;
	
	return (HashTable*)ht;
}

void hashtable_destroy(HashTable* hashtable)
{
	_HashTable* ht = (_HashTable*)hashtable;
	
	for (int i = 0; i < ht->tablesize; i++)
	{
		linkedlist_destroy(ht->table[i]);
	}
	
	free(ht->table);
	
	free(ht);
}

void* hashtable_get_element(HashTable* hashtable, void* e)
{
	_HashTable* ht = (_HashTable*)hashtable;
	
	LinkedList* ll = ht->table[ht->hash(e) % ht->tablesize];
	_BucketSearch bs;
	
	bs.hash = ht->hash;
	bs.equals = ht->equals;
	bs.key = e;
		
	void* element = linkedlist_get(ll, (LinkedListSearchFunction)bucket_search_function, &bs);
	
	return element;
}
	
void* hashtable_remove_element(HashTable* hashtable, void* e)
{
	_HashTable* ht = (_HashTable*)hashtable;
	
	LinkedList* ll = ht->table[ht->hash(e) % ht->tablesize];
	_BucketSearch bs;
	
	bs.hash = ht->hash;
	bs.equals = ht->equals;
	bs.key = e;
		
	void* element = linkedlist_remove(ll, (LinkedListSearchFunction)bucket_search_function, &bs);
	
	return element;
}
			
void hashtable_add(HashTable* hashtable, void* element)
{
	_HashTable* ht = (_HashTable*)hashtable;
								
	int hash = ht->hash(element) % ht->tablesize;
		
	LinkedList* ll = ht->table[hash];
		
	linkedlist_append(ll, element);
}

void hashtable_apply(HashTable* hashtable, HashTableApplyFunction func)
{
	_HashTable* ht = (_HashTable*)hashtable;
	
	for (int i = 0; i < ht->tablesize; i++)
	{
		linkedlist_apply(ht->table[i], (LinkedListApplyFunction)func);
	}
}

void hashtable_remove_all(HashTable* hashtable)
{
	_HashTable* ht = (_HashTable*)hashtable;
	
	for (int i = 0; i < ht->tablesize; i++)
	{
		linkedlist_remove_all(ht->table[i]);
	}
}

//****************************** Iterator ***********************

typedef struct
{
	_HashTable* hashtable;
	LinkedListIterator* iterator;
	int index;
} _HashTableIterator;

HashTableIterator* hashtableiterator_new(HashTable* hashtable)
{
	_HashTable* ht = (_HashTable*)hashtable;
	
	for (int i = 0; i < ht->tablesize; i++)
	{
		if (linkedlist_length(ht->table[i]) > 0)
		{
			_HashTableIterator* hti = (_HashTableIterator*)malloc(sizeof(_HashTableIterator));
	
			hti->hashtable = ht;
			hti->iterator = linkedlistiterator_new(ht->table[i]);
			hti->index = i;
		
			return (HashTableIterator*)hti;
		}
	}
	
	return NULL;	
}

void* hashtableiterator_next(HashTableIterator* iterator)
{	
	_HashTableIterator* hti = (_HashTableIterator*)iterator;
		
	void* element = linkedlistiterator_next(hti->iterator);
		
	if (element != NULL)
	{
		return element;
	}
						
	for (int i = hti->index + 1; i < hti->hashtable->tablesize; i++)
	{
		if (linkedlist_length(hti->hashtable->table[i]) > 0)
		{
			linkedlistiterator_destroy(hti->iterator);
			hti->iterator = linkedlistiterator_new(hti->hashtable->table[i]);
			hti->index = i;
			
			return linkedlistiterator_next(hti->iterator);
		}
	}
		
	return NULL;
}

void hashtableiterator_destroy(HashTableIterator* iterator)
{
	_HashTableIterator* hti = (_HashTableIterator*)iterator;
	
	linkedlistiterator_destroy(hti->iterator);
	free(hti);
}


















