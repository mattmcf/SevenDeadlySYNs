/* 
 * Queue.h -- public interface to the queue module
 */

#ifndef QUEUE_H
#define QUEUE_H


#ifdef __cplusplus
extern "C" {
#endif

typedef struct Queue Queue;

typedef void (*QueueApplyFunction)(void* elementp);
typedef int (*QueueSearchFunction)(void* elementp,void* keyp);
typedef int (*QueueFilterFunction)(void* elementp,void* userData);
typedef void* (*QueueMapFunction)(void* elementp);

/* create an empty queue */
Queue* queue_new();   

/* get the length of the queue */
int queue_length(Queue* qp);

/* deallocate a queue, assuming every element has been removed and deallocated */
void queue_destroy(Queue* qp);   

/* put element at end of queue */
void queue_push(Queue* qp, void* elementp); 

/* Pop an element off the end of the queue (like a stack) */
void* queue_spop(Queue* qu);
/* Get the element off the end of the queue (like a stack) but don't remove it */
void* queue_speek(Queue* qp);

/* get first element from a queue */
void* queue_pop(Queue* qp);

/* get the item at the given index */
void* queue_get(Queue* qp, int index);
void  queue_set(Queue* qp, void* element, int index);

/* apply a void function (e.g. a printing fn) to every element of a queue */
void queue_apply(Queue* qp, QueueApplyFunction af);
Queue* queue_filter(Queue* qp, QueueFilterFunction filter, void* userData);
Queue* queue_map(Queue* qp, QueueMapFunction map);

void queue_shuffle(Queue* qp);

/* search a queue using a supplied boolean function, returns an element */
void* queue_search(Queue* qp, QueueSearchFunction sf, void* skeyp);

/* search a queue using a supplied boolean function, removes an element */
void* queue_remove(Queue* qp, QueueSearchFunction sf, void* skeyp);

/* concatenatenates q2 onto q1, q2 may not be subsequently used */
void queue_concat(Queue* q1p, Queue* q2p);

#ifdef __cplusplus
}
#endif

#endif
