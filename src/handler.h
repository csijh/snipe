// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include "queue.h"
#include <stdbool.h>

// Provide event handling with multiple threads. Events from the system are
// pushed onto an event queue by the handle function. The handle function
// is called by the main thread, which then becomes the handler thread and does
// nothing else. The tick function pushes regular tick events for cursor
// blinking and autosave onto the event queue. It is called by a separate
// ticker thread which does nothing else. The main body of the program is run
// on a separate runner thread, which may call functions here to request the
// the handler thread to do something.

// TODO: find out what CTRL+ does, e.g. on keyboards where + is unshifted
// TODO: save on loss of focus.

// A handler manages the event queue.
struct handler;
typedef struct handler handler;

// Declare the event type, avoiding the inclusion of the event header.
typedef int event;

// Make a new handler, based on the given GLFW window, passed as a void pointer
// to prevent the GLFW/OpenGL APIs being exposed. Also pass the event queue,
// and the blink rate from the settings. Called on the main thread.
handler *newHandler(void *w, queue *q, double blinkRate);

// Free up the handler and its data. Called on the main thread.
void freeHandler(handler *h);

// Handle system events, including window resizing. Called on the main thread
// to form the handler thread. Return on QUIT.
void handle(handler *h);

// Ask the handler thread to resize the window. Called from the runner thread.
void resizeWindow(handler *h, int width, int height);

// Handle timer events. The argument is the handler, as a void pointer, and
// the result is NULL, for compatibility with pthread creation. Called from a
// separate ticker thread. Return on QUIT.
void *tick(void *vh);

// Check whether the window has the input focus. Called from the runner thread.
// TODO: replace with FOCUS and DEFOCUS events.
bool focused(handler *h);

// Get the next event, possibly with a pause. Fill in the px, py, pt variables.
// Called from the runner thread.
event getRawEvent(handler *h, int *px, int *py, char const **pt);

// Generate a frame event. Call when drawing a frame with vsync.
//void frameEvent(handler *h);

// Generate a paste event. Called from the runner thread when C_V is pressed.
void pasteEvent(handler *h);

// Copy text on CUT/COPY from the document back to the OS. Called from the
// runner thread.
void clip(handler *h, char const *text);
