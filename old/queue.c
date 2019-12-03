// The Snipe editor is free and open source, see licence.txt.
#define _POSIX_C_SOURCE 200809L
#include "queue.h"
#include "event.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

// A point is an x,y pair of pixel coordinates.
struct point { int x, y; };
typedef struct point point;

// Size of queue and size of in-place event text (must be >= 5 for
// maximum-length UTF8 code point.
enum { QUEUE_SIZE = 512, TEXT_SIZE = 8 };

// An event structure holds an event, with coordinates or text or a string.
struct data {
    event e;
    union { point p; char t[TEXT_SIZE]; char *s; };
};
typedef struct data data;

// A queue is a circular array with a lock to share it between threads.
// Two condition variables are used to make threads wait to push or pull.
// There is a string buffer to retain the text data for the most recent
// event until the next event is requested. Animation frame events are
// not pushed on the array, but just counted, and returned when the array
// is otherwise empty.
struct queue {
    int size, head, tail;
    data *array;
    pthread_mutex_t lock;
    pthread_cond_t pushable;
    pthread_cond_t pullable;
};

// Check that a result from a pthread function is zero.
static inline void Z(int r) {
    if (r == 0) return;
    printf("Pthread function failed\n");
    exit(1);
}

queue *newQueue() {
    queue *q = malloc(sizeof(queue));
    q->size = QUEUE_SIZE;
    q->head = q->tail = 0;
    q->array = malloc(q->size * sizeof(data));
    Z(pthread_cond_init(&q->pushable, NULL));
    Z(pthread_cond_init(&q->pullable, NULL));
    Z(pthread_mutex_init(&q->lock, NULL));
    return q;
}

void freeQueue(queue *q) {
    free(q->array);
    free(q);
}

// Check if the queue is empty.
static inline bool empty(queue *q) {
    return q->head == q->tail;
}

// Check if the queue is full. The last slot is unused, otherwise full would be
// indistinguishable from empty. Also, not using the last slot ensures that the
// data for an event can't get overwritten while that event is being processed.
static inline bool full(queue *q) {
    return ((q->head + 1) % q->size) == q->tail;
}

// Pull an event from a non-empty queue, returning its slot pointer.
static inline data *pull(queue *q) {
    data *d = &q->array[q->tail];
    q->tail = (q->tail + 1) % q->size;
    return d;
}

// Prepare to push an event to a non-full queue, returning the slot pointer.
static inline data *push(queue *q) {
    data *d = &q->array[q->head];
    q->head = (q->head + 1) % q->size;
    return d;
}

// Look at the previous event on a non-empty queue.
static inline data *previous(queue *q) {
    int i = q->head - 1;
    if (i < 0) i = q->size - 1;
    return &q->array[i];
}

// Attempt to merge the event with the most recently added event.
static bool combine(queue *q, event e, int x, int y) {
    if (empty(q)) return false;
    data *p = previous(q);
    if (p->e != e) return false;
    switch (e) {
        case FRAME:
            return true;
        case RESIZE: case DRAG:
            p->p.x = x;
            p->p.y = y;
            return true;
        case SCROLL:
            p->p.x += x;
            p->p.y += y;
            return true;
        default:
            return false;
    }
}

// Push an event, waiting if necessary. If adding to an empty queue, wake up any
// threads waiting to pull. For a FRAME event, count it rather than queueing it.
void enqueue(queue *q, event e, int x, int y, char const *t) {
    pthread_mutex_lock(&q->lock);
    if (combine(q, e, x, y)) {
        pthread_mutex_unlock(&q->lock);
        return;
    }
    while (full(q)) pthread_cond_wait(&q->pushable, &q->lock);
    bool tell = empty(q);
    data *d = push(q);
    d->e = e;
    if (e == CLICK || e == DRAG || e == SCROLL) {
        d->p.x = x; d->p.y = y;
    }
    else if (e == TEXT) strcpy(d->t, t);
    else if (e == PASTE) d->s = (char *) t;
    if (tell) pthread_cond_broadcast(&q->pullable);
    pthread_mutex_unlock(&q->lock);
}

// Pull an event, waiting if necessary. If pulling from a full queue, wake up
// threads waiting to push. Allow non-FRAME events to overtake FRAME events, by
// returning a FRAME event only if the queue is otherwise empty. For TEXT, it is
// OK to return a pointer to the eventData structure, because further enqueued
// events can't overwrite the current data before the next event is requested.
// Buffer text to keep it valid until the next request. If two DRAG events
// have arrived from a fast mouse movement discard the first.
event dequeue(queue *q, int *px, int *py, char const **pt) {
    pthread_mutex_lock(&q->lock);
    while (empty(q)) pthread_cond_wait(&q->pullable, &q->lock);
    bool tell = full(q);
    data *d = pull(q);
    event e = d->e;
    if (e != FRAME) {
        if (! combine(q, FRAME, 0, 0)) {
            data *d = push(q);
            d->e = FRAME;
        }
    }
    if (e == CLICK || e == DRAG || e == SCROLL) {
        *px = d->p.x; *py = d->p.y;
    }
    else if (e == TEXT) *pt = d->t;
    else if (e == PASTE) *pt = d->s;
    if (tell) pthread_cond_broadcast(&q->pushable);
    pthread_mutex_unlock(&q->lock);
    return e;
}

#ifdef queueTest

int main() {
    setbuf(stdout, NULL);
    queue *q = newQueue();
    enqueue(q, TEXT, 0, 0, "a");
    enqueue(q, TEXT, 0, 0, "b");
    event e;
    int x, y;
    char const *t;
    e = dequeue(q, &x, &y, &t);
    assert(e == TEXT && strcmp(t, "a") == 0);
    e = dequeue(q, &x, &y, &t);
    assert(e == TEXT && strcmp(t, "b") == 0);
    freeQueue(q);
    printf("Queue module OK\n");
}

#endif
