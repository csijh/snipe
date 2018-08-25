// The Snipe editor is free and open source, see licence.txt.

// An event structure holds a tag and two coordinates or a string or character.
struct position { int x, y; };
struct eventData {
    event tag;
    union { struct position p; char const *s; char c[5]; };
};
typedef struct eventData eventData;

// Queue.
struct queue {
    int length, start, end;
    eventData *events;
    pthread_mutex_t lock;
    pthread_cond_t ready;
    char *buffer;
};

newQueue() {
    q = ...
    Z(pthread_cond_init(&q->ready, NULL));
    Z(pthread_mutex_init(&q->lock, NULL));
    return q;
}

freeQueue(queue *q) {

}

// If adding to an empty queue, wake up any waiting thread.
enqueue(queue *q, ...) {
    pthread_mutex_lock(&q->lock);
    if (q->length == 0) pthread_cond_broadcast(&q->ready);
    q->length++;
    pthread_mutex_unlock(&q->lock);
}

dequeue() {
    pthread_mutex_lock(&q->lock);
    while (empty) {
        pthread_cond_wait(&q->ready, &q->lock);
    }
    msg = pull();
    pthread_mutex_unlock(&q->lock);
    return msg;
}
