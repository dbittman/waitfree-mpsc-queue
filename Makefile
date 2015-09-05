CFLAGS=-Wall -Wextra -Werror -std=c11 -O2
LDFLAGS=-lpthread -pg
all: libmpscq.so libmpscq.a mpsc_test

mpsc_test: mpscq.h mpsc.o mpsc_test.o

mpsc_test.o: mpsc_test.c

mpsc.o: mpsc.c mpscq.h

libmpscq.a: mpsc.o
	ar -cvq libmpscq.a mpsc.o

libmpscq.so: mpsc.c mpscq.h Makefile
	$(CC) $(CFLAGS) -shared -o libmpscq.so -fPIC mpsc.c

clean:
	-rm libmpscq.* *.o mpsc_test
