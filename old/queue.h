// The Snipe editor is free and open source, see licence.txt.

// An event queue is shared between threads. It has a fixed size so that the
// operating system can detect if the program stops responding.

// Any thread can push an event onto the queue. The runner thread pulls events
// off the queue. The queue helps treat all situations, including animations, as
// event loops using timer and frame events as well as keyboard/mouse/window
// system events. A FRAME event indicates that the current state of the program
// should be drawn onto the screen. All other events just update the state.
// To create an animation, before or at each FRAME event, the runner thread
// pushes some event onto the queue representing the next item of work to be
// done. Whenever a non-frame event is pulled off the queue, a FRAME event is
// pushed so that the updates for all outstading events get drawn.

// To deal with frames with no changes, and rapidly generated events, two
// successive FRAME or RESIZE or DRAG or SCROLL events of the same type pushed
// onto the queue are amalgamated by discarding the earlier one or, in the case
// of SCROLL, by adding the scroll amounts.
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
