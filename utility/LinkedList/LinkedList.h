#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LinkedList LinkedList;

typedef int (*LinkedListSearchFunction)(void* element, void* key);
typedef void (*LinkedListApplyFunction)(void* element);

LinkedList* linkedlist_new();
void linkedlist_destroy(LinkedList* list);

void linkedlist_append(LinkedList* list, void* element);
void* linkedlist_remove(LinkedList* list, LinkedListSearchFunction f, void* element);
void* linkedlist_get(LinkedList* list, LinkedListSearchFunction f, void* element);
int linkedlist_length(LinkedList* list);
void linkedlist_apply(LinkedList* list, LinkedListApplyFunction func);
void linkedlist_remove_all(LinkedList* list);

typedef struct LinkedListIterator LinkedListIterator;

LinkedListIterator* linkedlistiterator_new(LinkedList* list);
void* linkedlistiterator_next(LinkedListIterator* iterator);
void* linkedlistiterator_current(LinkedListIterator* iterator);
void* linkedlistiterator_previous(LinkedListIterator* iterator);
int linkedlistiterator_insert_before(LinkedListIterator* iterator, void* element);
int linkedlistiterator_insert_after(LinkedListIterator* iterator, void* element);
void* linkedlistiterator_remove(LinkedListIterator* iterator);
void linkedlistiterator_reset(LinkedListIterator* iterator);
int linkedlistiterator_index(LinkedListIterator* iterator);

void linkedlistiterator_destroy(LinkedListIterator* iterator);

#ifdef __cplusplus
}
#endif

#endif
