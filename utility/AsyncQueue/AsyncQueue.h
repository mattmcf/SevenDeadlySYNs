#ifndef ASYNCQUEUE_H
#define ASYNCQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../Queue/Queue.h"
#include <pthread.h>
	
typedef struct AsyncQueue AsyncQueue;
	
/* create an empty queue */
AsyncQueue* asyncqueue_new();        

/* get the length of the queue */
int asyncqueue_length(AsyncQueue* qp);

/* deallocate a queue, assuming every element has been removed and deallocated */
void asyncqueue_destroy(AsyncQueue* qp);   

/* put element at end of queue */
void asyncqueue_push(AsyncQueue* qp, void* elementp); 

/* get first element from a queue */
void* asyncqueue_pop(AsyncQueue* qp);

/* apply a void function (e.g. a printing fn) to every element of a queue */
void asyncqueue_apply(AsyncQueue* qp, QueueApplyFunction af);

/* search a queue using a supplied boolean function, returns an element */
void* asyncqueue_search(AsyncQueue* qp, QueueSearchFunction sf, void* skeyp);

/* search a queue using a supplied boolean function, removes an element */
void* asyncqueue_remove(AsyncQueue* qp, QueueSearchFunction sf, void* skeyp);

/* concatenatenates q2 onto q1, q2 may not be subsequently used */
void asyncqueue_concat(AsyncQueue* q1p, AsyncQueue* q2p);


#ifdef __cplusplus
}
#endif

#endif

