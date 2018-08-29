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

// Size of text to support. Must be >= 5 for maximum-length UTF8 code point.
enum { TEXT_SIZE = 8 };

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
    int bufferSize;
    char *buffer;
    int frames;
};

// Check that a result from a pthread function is zero.
static inline void Z(int r) {
    if (r == 0) return;
    printf("Pthread function failed\n");
    exit(1);
}

queue *newQueue() {
    queue *q = malloc(sizeof(queue));
    q->size = 1024;
    q->head = q->tail = 0;
    q->array = malloc(q->size * sizeof(data));
    q->bufferSize = 256;
    q->buffer = NULL;
    q->frames = 0;
    assert(q->bufferSize >= TEXT_SIZE);
    Z(pthread_cond_init(&q->pushable, NULL));
    Z(pthread_cond_init(&q->pullable, NULL));
    Z(pthread_mutex_init(&q->lock, NULL));
    return q;
}

void freeQueue(queue *q) {
    free(q->array);
    free(q->buffer);
    free(q);
}

// Check if the queue is empty.
static inline bool empty(queue *q) {
    return q->head == q->tail;
}

// Check if the queue is full.
static inline bool full(queue *q) {
    return ((q->head + 1) % q->size) == q->tail;
}

// Pull an event from a non-empty queue, returning its slot pointer.
static inline data *pull(queue *q) {
    data *d = &q->array[q->tail];
    q->tail = (q->tail + 1) % q->size;
    return d;
}

// Prepare to push an event to a non-full queue, returning the next slot.
static inline data *push(queue *q) {
    data *d = &q->array[q->head];
    q->head = (q->head + 1) % q->size;
    return d;
}

// Push an event, waiting if necessary. If adding to an empty queue, wake up any
// threads waiting to pull. For a FRAME event, count it rather than queueing it.
void enqueue(queue *q, event e, int x, int y, char const *t) {
    pthread_mutex_lock(&q->lock);
    if (e == FRAME) {
        q->frames++;
        pthread_mutex_unlock(&q->lock);
        return;
    }
    while (full(q)) pthread_cond_wait(&q->pushable, &q->lock);
    bool tell = empty(q);
    data *d = push(q);
    d->e = e;
    if (e == CLICK || e == DRAG) { d->p.x = x; d->p.y = y; }
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

// Buffer text to
// keep it valid until the next request.
event dequeue(queue *q, int *px, int *py, char const **pt) {
    pthread_mutex_lock(&q->lock);
    if (empty(q) && q->frames > 0) {
        q->frames--;
        pthread_mutex_unlock(&q->lock);
        return FRAME;
    }
    while (empty(q)) pthread_cond_wait(&q->pullable, &q->lock);
    bool tell = full(q);
    data *d = pull(q);
    event e = d->e;
    if (e == CLICK || e == DRAG) { *px = d->p.x; *py = d->p.y; }
    else if (e == TEXT) *pt = d->t;
    else if (e == PASTE) {
        q->buffer = d->s;
//        int n = strlen(d->s) + 1;
//        if (q->bufferSize < n) {
//            q->bufferSize = n;
//            free(q->buffer);
//            q->buffer = malloc(n);
//        }
//        strcpy(q->buffer, d->s);
//        free(d->s);
        *pt = q->buffer;
    }
    if (tell) pthread_cond_broadcast(&q->pushable);
    pthread_mutex_unlock(&q->lock);
    return e;
}

#ifdef test_queue

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
