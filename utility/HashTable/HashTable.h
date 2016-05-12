#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef int  (*HashFunction)(void* element);
typedef int  (*ElementEqualsFunction)(void* e0, void* e1);
typedef void (*HashTableApplyFunction)(void* element);

typedef struct HashTable HashTable;

HashTable* hashtable_new(HashFunction hashfunc, ElementEqualsFunction equals);
void hashtable_destroy(HashTable* hashtable);

void* hashtable_remove_element(HashTable* hashtable, void* element);
void* hashtable_get_element(HashTable* hashtable, void* element);

void hashtable_add(HashTable* hashtable, void* element);
int hashtable_size(HashTable* hashtable);
void hashtable_apply(HashTable* hashtable, HashTableApplyFunction func);
void hashtable_remove_all(HashTable* hashtable);

typedef struct HashTableIterator HashTableIterator;

HashTableIterator* hashtableiterator_new(HashTable* hashtable);
void* hashtableiterator_next(HashTableIterator* iterator);
void hashtableiterator_destroy(HashTableIterator* iterator);

#endif
