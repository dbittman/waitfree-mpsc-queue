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

void *producer_main(void *x)
{
	int num = 20;
	for(int i=0;i<num;i++) {
		bool r = mpscq_enqueue(queue, x);
		if(r) {
			atomic_fetch_add(&amount_produced, 1);
		} else {
			i--;
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

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	int num_producers = 20;
	pthread_t producers[num_producers];
	pthread_t consumer;

	queue = mpscq_create(NULL, 8000);

	fprintf(stderr, ":: LOCK FREE? %d %d %d %d\n", ATOMIC_INT_LOCK_FREE, ATOMIC_LONG_LOCK_FREE, ATOMIC_POINTER_LOCK_FREE, ATOMIC_LLONG_LOCK_FREE);

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

