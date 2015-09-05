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
#define NUM_ITEMS 100
#define NUM_THREADS 128

void *producer_main(void *x)
{
	struct timespec start, end;
	for(int i=0;i<NUM_ITEMS;i++) {
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
		bool r = mpscq_enqueue(queue, x);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		if(r) {
			atomic_fetch_add(&amount_produced, 1);
			total += (end.tv_nsec - start.tv_nsec) / 1000;
		} else {
			i--;
			retries++;
		}
	}
	pthread_exit(0);
}

void *consumer_main(void *x)
{
	(void)x;
	while(true) {
		void *ret = mpscq_dequeue(queue);
		if(ret) {
			atomic_fetch_add(&amount_consumed, 1);
		}
		if(!ret && done) {
			if(!mpscq_dequeue(queue))
				break;
			else
				atomic_fetch_add(&amount_consumed, 1);
		}
	}
	assert(!mpscq_dequeue(queue));
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

	int cap = atoi(argv[1]);
	queue = mpscq_create(NULL, cap);

	pthread_create(&consumer, NULL, consumer_main, NULL);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	for(long i=0;i<num_producers;i++) {
		pthread_create(&producers[i], NULL, producer_main, &producers[i]);
	}

	for(int i=0;i<num_producers;i++) {
		pthread_join(producers[i], NULL);
	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
	done = true;
	pthread_join(consumer, NULL);

	long ms = (end.tv_sec - start.tv_sec) * 1000;
	ms += (end.tv_nsec - start.tv_nsec) / 1000000;

	fprintf(stdout, "\t%d\t%ld\t%ld\n",
			retries, ms, (long)(total / amount_produced));
	exit(amount_produced != amount_consumed);
}

