#ifndef __MPSCQ_H
#define __MPSCQ_H

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/types.h>

#define MPSCQ_MALLOC 1

struct mpscq {
	_Atomic size_t count;
	_Atomic size_t head;
	size_t tail;
	size_t max;
	void * _Atomic *buffer;
	int flags;
};


struct mpscq *mpscq_create(struct mpscq *n, size_t capacity);
bool mpscq_enqueue(struct mpscq *q, void *obj);
void *mpscq_dequeue(struct mpscq *q);
size_t mpscq_count(struct mpscq *q);
size_t mpscq_capacity(struct mpscq *q);
void mpscq_destroy(struct mpscq *q);


#endif

