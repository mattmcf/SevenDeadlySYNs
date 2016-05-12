#include "AsyncQueue.h"
#include <stdlib.h>

#define synchronize(lock, code) {pthread_mutex_lock(lock); code; pthread_mutex_unlock(lock);}

typedef struct
{
	Queue* q;
	pthread_mutex_t* lock;
} _AsyncQueue;

AsyncQueue* asyncqueue_new()
{
	_AsyncQueue* aq = (_AsyncQueue*)malloc(sizeof(_AsyncQueue));
	
	aq->q = queue_new();
	aq->lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(aq->lock, NULL);
	
	return (AsyncQueue*)aq;
}     

int asyncqueue_length(AsyncQueue* qp)
{
	_AsyncQueue* aq = (_AsyncQueue*)qp;
	
	int len;
	synchronize(aq->lock,
		len = queue_length(aq->q);
	)
	
	return len;	
}

void asyncqueue_destroy(AsyncQueue* qp)
{
	_AsyncQueue* aq = (_AsyncQueue*)qp;
	
	synchronize(aq->lock,
		queue_destroy(aq->q);
	)

	pthread_mutex_destroy(aq->lock);
	free(aq);
}   

void asyncqueue_push(AsyncQueue* qp, void* elementp)
{
	_AsyncQueue* aq = (_AsyncQueue*)qp;
	
	synchronize(aq->lock,
		queue_push(aq->q, elementp);
	)
} 

void* asyncqueue_pop(AsyncQueue* qp)
{
	_AsyncQueue* aq = (_AsyncQueue*)qp;
	
	void* element;
	synchronize(aq->lock,
		element = queue_pop(aq->q);
	)
	
	return element;
}

void asyncqueue_apply(AsyncQueue* qp, QueueApplyFunction af)
{
	_AsyncQueue* aq = (_AsyncQueue*)qp;
	
	synchronize(aq->lock,
		queue_apply(aq->q, af);
	)
}

void* asyncqueue_search(AsyncQueue* qp, QueueSearchFunction sf, void* skeyp)
{
	_AsyncQueue* aq = (_AsyncQueue*)qp;
	
	void* element;
	synchronize(aq->lock,
		element = queue_search(aq->q, sf, skeyp);
	)
	
	return element;
}

void* asyncqueue_remove(AsyncQueue* qp, QueueSearchFunction sf, void* skeyp)
{
	_AsyncQueue* aq = (_AsyncQueue*)qp;
	
	void* element;
	synchronize(aq->lock,
		element = queue_remove(aq->q, sf, skeyp);
	)
	
	return element;
}

void asyncqueue_concat(AsyncQueue* q1p, AsyncQueue* q2p)
{
	_AsyncQueue* q1 = (_AsyncQueue*)q1p;
	_AsyncQueue* q2 = (_AsyncQueue*)q2p;
	
	synchronize(q1->lock,
		synchronize(q2->lock,
			queue_concat((Queue*)q1, (Queue*)q1);
		)
	)
}
