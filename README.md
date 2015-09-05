MPSCQ - Multiple Producer, Single Consumer Wait-Free Queue
==========================================================
C11 library that allows multiple threads to enqueue something to a queue, and allows one thread (and only one thread) to dequeue from it.

This code is tested, but not proven. Use it at your own peril.

Interface
---------
Creation and destruction of a queue can be done with:

    struct mpscq *mpscq_create(struct mpscq *n, size_t capacity);
    void mpscq_destroy(struct mpscq *q);

Passing a NULL pointer as _n_ will allocate a new queue with malloc, initialize it, and return it. Passing a pointer to a struct mpscq as _n_ will initialize that object. Calling the destroy function will free the internal data of the object, and if the object was allocated via malloc, it will be freed as well.

Enqueuing can be done with:

    bool mpscq_enqueue(struct mpscq *q, void *obj);

which will enqueue _obj_ in _q_, returning true if it was enqueued and false if it wasn't (queue was full).

Dequeuing can be done with:

    void *mpscq_dequeue(struct mpscq *q);

which will return NULL if the queue was empty or an object from the queue if it wasn't. Note that
a queue may appear to be empty if a thread is in the process if writing the object in the next slot in the buffer, but that's okay because the function can be called again (see the comments in the source for more interesting comments on this).

The queue may also be queried for current number of items and for total capacity:

    size_t mpscq_capacity(struct mpscq *q);
    size_t mpscq_count(struct mpscq *q);

Comments
--------
PLEASE report bugs to me if you find any (email me at danielbittman1@gmail.com).

Technical Details
-----------------
During the first half of the enqueuing function, we prevent writing to the queue if the queue is full. This is done by doing an add anyway, and then seeing if the old value was greater than or equal to max. If so, then we cannot write to the queue because it's full. This is safe for multiple threads, since the worst thing that can happen is a thread sees the count to be way above the max. This is okay, since it'll just report the queue as being full.

The second half of the enqueuing function gains near-exclusive access to the head element. It isn't completely exclusive, since the consumer thread may be observing that element. However, we prevent any producer threads from trying to write to the same area of the queue. Once head is fetched and incremented, we store the object to the head location, thus releasing that memory location.

In the dequeue function, we exchange the tail with NULL, and observe the return value. If the return value is NULL, then there's nothing in the queue and so we return NULL. If we got back an object, we just increment the tail and decrement the count, before returning.

Performance (preliminary)
-------------------------
Here's a quick comparison to a locked circular queue I wrote quickly, fueled by beer. With 64 threads, each writing 200 objects to the queue with the speed of 64 fairly slow threads (and, of course, a singular thread reading from it with the speed of a one fairly slow thread... ) the lock-free queue wins pretty convincingly:

![I WILL WRITE 500 OBJECTS, AND I WILL WRITE 500 MORE](https://raw.githubusercontent.com/dbittman/waitfree-mpsc-queue/master/data/64-200.png)

(hard to see: the left-most data points are at x=50, not 0)

Well, that's pretty nice. If your queue is small, then MPSCQ does wonders compared to locking, which is what I would expect.

