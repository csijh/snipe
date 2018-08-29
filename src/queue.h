// The Snipe editor is free and open source, see licence.txt.

// An event queue is shared between threads. It has a fixed size so that it
// doesn't expand indefinitely and prevent the operating system from detecting
// that the program isn't responding. Since an animation FRAME event causes
// an actual (1/60 sec) delay, other events are allowed to overtake it.
struct queue;
typedef struct queue queue;

// Create or free a queue.
queue *newQueue();
void freeQueue(queue *q);

// Define the event enum without including event.h.
typedef int event;

// Push an event onto the queue with any associated pixel coordinates or text.
// The text is copied.
void enqueue(queue *h, event e, int x, int y, char const *t);

// Get the next event, when available. The x, y and t variables are filled in
// via the provided pointers. The text t is only valid until the next call.
event dequeue(queue *q, int *px, int *py, char const **pt);
