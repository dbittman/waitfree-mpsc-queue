#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

struct mpsc {
	_Atomic int count;
	_Atomic int head;
	_Atomic int tail;
	int max;
	void * _Atomic *buffer; //buffer is a pointer to atomic void *
};

struct mpsc *mpsc_new(void)
{
	struct mpsc *n = malloc(sizeof(*n));
	n->count = ATOMIC_VAR_INIT(0);
	n->head = ATOMIC_VAR_INIT(0);
	n->tail = ATOMIC_VAR_INIT(0);
	n->buffer = malloc(4096 * sizeof(void *));
	n->max = 4096;
	return n;
}

struct mpsc *queue;
_Atomic int amount_produced = ATOMIC_VAR_INIT(0);
_Atomic int amount_consumed = ATOMIC_VAR_INIT(0);
_Atomic bool done = ATOMIC_VAR_INIT(false);

int mpsc_enqueue(struct mpsc *q, void *obj)
{
	int count = atomic_fetch_add(&q->count, 1);
	if(count >= q->max) {
		atomic_fetch_sub(&q->count, 1);
		return -1;
	}

	bool result;
	int extras = 0;
	do {
		void *expected = NULL;
		int head = atomic_fetch_add(&q->head, 1);
		if(head > q->max && 0) {
			int new_head = head + 1;
			bool r = atomic_compare_exchange_strong(&q->head, &new_head, 0);
			if(r)
				fprintf(stderr, "------- RESET HEAD\n");
		}
		result = atomic_compare_exchange_strong(&q->buffer[head % q->max], &expected, obj);
		if(!result)
			atomic_fetch_sub(&q->head, 1);
		++extras;
	} while(!result);
	return extras - 1;
}

void *mpsc_dequeue(struct mpsc *q)
{
	int count = atomic_load(&q->count);
	if(count == 0)
		return NULL;
	void *ret = atomic_exchange(&q->buffer[q->tail], NULL);
	if(!ret)
		return NULL;
	if(++q->tail >= q->max)
		q->tail = 0;
	atomic_fetch_sub(&q->count, 1);
	return ret;
}

void *producer_main(void *x)
{
	int fails = 0, succ=0;
	int num = 200;
	for(int i=0;i<num;i++) {
		int r = mpsc_enqueue(queue, x);
		if(r >= 0) {
			fails+=r;
			succ++;
			atomic_fetch_add(&amount_produced, 1);
		} else {
			i--;
		}
	}
	if(fails > 0)
		printf("Producer produced %d items, failed %d times (%f rate)\n", succ, fails, (float)fails / num);
	pthread_exit(0);
}

void *consumer_main(void *x)
{
	(void)x;
	while(true) {
		void *ret = mpsc_dequeue(queue);
		if(ret) {
			atomic_fetch_add(&amount_consumed, 1);
		}
		if(!ret && done) {
			if(!mpsc_dequeue(queue))
				break;
			else
				atomic_fetch_add(&amount_consumed, 1);
		}
	}
	assert(!mpsc_dequeue(queue));
	pthread_exit(0);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	int num_producers = 200;
	pthread_t producers[num_producers];
	pthread_t consumer;

	queue = mpsc_new();

	pthread_create(&consumer, NULL, consumer_main, NULL);
	for(long i=0;i<num_producers;i++) {
		pthread_create(&producers[i], NULL, producer_main, &producers[i]);
	}

	for(int i=0;i<num_producers;i++) {
		pthread_join(producers[i], NULL);
	}
	done = true;
	pthread_join(consumer, NULL);
	
	fprintf(stderr, "Amount produced: %d, Amount consumed: %d\n",
			amount_produced, amount_consumed);
	exit(amount_produced != amount_consumed);
}

