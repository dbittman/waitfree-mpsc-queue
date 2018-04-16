/* 2015 Daniel Bittman <danielbittman1@gmail.com>: http://dbittman.github.io/ */
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include "mpscq.h"

struct mpscq *queue;
_Atomic int amount_produced = ATOMIC_VAR_INIT(0);
_Atomic int amount_consumed = ATOMIC_VAR_INIT(0);
_Atomic bool done = ATOMIC_VAR_INIT(false);
_Atomic int retries = ATOMIC_VAR_INIT(0);
_Atomic long long total = ATOMIC_VAR_INIT(0);
#define NUM_ITEMS 10000
#define NUM_THREADS 32

struct item {
	_Atomic int sent, recv;
};

struct item items[NUM_THREADS][NUM_ITEMS];

void *producer_main(void *x)
{
	long tid = (long)x;
	struct timespec start, end;
	for(int i=0;i<NUM_ITEMS;i++) {
		assert(atomic_fetch_add(&items[tid][i].sent, 1) == 0);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
		bool r = mpscq_enqueue(queue, &items[tid][i]);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		if(r) {
			atomic_fetch_add(&amount_produced, 1);
			total += (end.tv_nsec - start.tv_nsec) / 1000;
		} else {
			items[tid][i].sent = 0;
			i--;
			retries++;
		}
	}
	for(int i=0;i<NUM_ITEMS;i++) {
		assert(items[tid][i].sent != 0);
	}
	atomic_thread_fence(memory_order_seq_cst);
	pthread_exit(0);
}

void *consumer_main(void *x)
{
	(void)x;
	bool doublechecked = false;
	while(true) {
		void *ret = mpscq_dequeue(queue);
		if(ret) {
			atomic_fetch_add(&amount_consumed, 1);
			struct item *it = ret;
			assert(atomic_fetch_add(&it->sent, 1) == 1);
			assert(atomic_fetch_add(&it->recv, 1) == 0);
			doublechecked = false;
		} else if(done && doublechecked) {
			break;
		} else if(done) {
			doublechecked = true;
		}
	}
	assert(!mpscq_dequeue(queue));
	atomic_thread_fence(memory_order_seq_cst);
	assert(queue->count == 0);
	assert(queue->head % queue->max == queue->tail);
	pthread_exit(0);
}

#include <time.h>
int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	int num_producers = NUM_THREADS-1;
	pthread_t producers[num_producers];
	pthread_t consumer;

	struct timespec start, end;

	for(int i=0;i<NUM_THREADS;i++) {
		for(int j=0;j<NUM_ITEMS;j++) {
			items[i][j].sent = 0;
			items[i][j].recv = 0;
		}
	}

	int cap = atoi(argv[1]);
	queue = mpscq_create(NULL, cap);

	pthread_create(&consumer, NULL, consumer_main, NULL);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	for(long i=0;i<num_producers;i++) {
		pthread_create(&producers[i], NULL, producer_main, (void *)i);
	}

	for(int i=0;i<num_producers;i++) {
		pthread_join(producers[i], NULL);
	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
	done = true;
	pthread_join(consumer, NULL);

	atomic_thread_fence(memory_order_seq_cst);
	
	for(int i=0;i<num_producers;i++) {
		for(int j=0;j<NUM_ITEMS;j++) {
			if(items[i][j].sent != 2) {
				printf(":(%d %d): %d %d, %d %d\n", i, j, items[i][j].sent,
						items[i][j].recv, amount_produced, amount_consumed);
			}
			assert(items[i][j].sent == 2);
			assert(items[i][j].recv == 1);
		}
	}
	
	long ms = (end.tv_sec - start.tv_sec) * 1000;
	ms += (end.tv_nsec - start.tv_nsec) / 1000000;

	fprintf(stdout, "\t%d\t%ld\t%ld\n",
			retries, ms, (long)(total / amount_produced));
	assert(amount_produced == amount_consumed);
	exit(amount_produced != amount_consumed);
}

