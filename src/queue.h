// The Snipe editor is free and open source, see licence.txt.

// An event queue is shared between threads. It has a fixed size so that, if
// the main body of the program gets stuck, the operating system will detect it.
struct queue;
typedef struct queue queue;

queue *newQueue();
void freeQueue(queue *q);

// Define the event enum without including event.h.
typedef int event;

// Push an event onto the queue with any associated pixel coordinates or text.
// The text is copied.
void enqueue(queue *h, event e, int x, int y, char const *t);

// Get the next event, blocking until it is available.
event dequeue(queue *q, int *x, int *y, char const **t);
