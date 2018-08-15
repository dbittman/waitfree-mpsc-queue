# 2015 Daniel Bittman <danielbittman1@gmail.com>: http://dbittman.github.io/

CFLAGS=-Wall -Wextra -Werror -std=gnu11 -O3
LDFLAGS=
LDLIBS=-lpthread
CC=gcc

ifeq ($(strip $(DEBUG)),tsan)
	CC=clang
	CFLAGS+=-fsanitize=thread
	LDFLAGS+=-fsanitize=thread
else ifeq ($(strip $(DEBUG)),asan)
	CC=clang
	CFLAGS+=-fsanitize=address
	LDFLAGS+=-fsanitize=address
endif

all: libmpscq.so libmpscq.a mpsc_test

mpsc_test: mpsc.o mpsc_test.o

mpsc_test.o: mpsc_test.c

mpsc.o: mpsc.c mpscq.h

libmpscq.a: mpsc.o
	ar -cvq libmpscq.a mpsc.o

libmpscq.so: mpsc.c mpscq.h Makefile
	$(CC) $(CFLAGS) -shared -o libmpscq.so -fPIC mpsc.c

clean:
	-rm libmpscq.* *.o mpsc_test

prof: mpsc_test
	for i in $$(seq 1 100); do sudo nice -n -20 ./mpsc_test; mv -f gmon.out gmon.out.$$i; done
	gprof -s ./mpsc_test gmon.out.*;  gprof ./mpsc_test gmon.sum -bQ

