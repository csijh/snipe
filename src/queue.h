// The Snipe editor is free and open source, see licence.txt.

// An event queue, shared between threads.
struct queue;
typedef struct queue queue;

queue *newQueue();
void freeQueue(queue *q);

// Push an event onto the queue with any associated pixel coordinates or text.
void enqueue(queue *h, event e, int x, int y, char const *t);

// Get the next event. Block until it is available.
event dequeue(queue *q, int *x, int *y, char const **t);
