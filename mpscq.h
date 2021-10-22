/* 2015 Daniel Bittman <danielbittman1@gmail.com>: http://dbittman.github.io/ */
#ifndef __MPSCQ_H
#define __MPSCQ_H

#include <stdint.h>
#ifndef __cplusplus
#include <stdatomic.h>
#endif
#include <stdbool.h>
#include <sys/types.h>

#define MPSCQ_MALLOC 1

#ifndef __cplusplus
struct mpscq {
	_Atomic size_t count;
	_Atomic size_t head;
	size_t tail;
	size_t max;
	void * _Atomic *buffer;
	int flags;
};
#else
struct mpscq;
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* create a new mpscq. If n == NULL, it will allocate
 * a new one and return it. If n != NULL, it will
 * initialize the structure that was passed in. 
 * capacity must be greater than 1, and it is recommended
 * to be much, much larger than that. It must also be a power of 2. */
struct mpscq *mpscq_create(struct mpscq *n, size_t capacity);

/* enqueue an item into the queue. Returns true on success
 * and false on failure (queue full). This is safe to call
 * from multiple threads */
bool mpscq_enqueue(struct mpscq *q, void *obj);

/* dequeue an item from the queue and return it.
 * THIS IS NOT SAFE TO CALL FROM MULTIPLE THREADS.
 * Returns NULL on failure, and the item it dequeued
 * on success */
void *mpscq_dequeue(struct mpscq *q);

/* get the number of items in the queue currently */
size_t mpscq_count(struct mpscq *q);

/* get the capacity of the queue */
size_t mpscq_capacity(struct mpscq *q);

/* destroy a mpscq. Frees the internal buffer, and
 * frees q if it was created by passing NULL to mpscq_create */
void mpscq_destroy(struct mpscq *q);

#ifdef __cplusplus
}
#endif

#endif

