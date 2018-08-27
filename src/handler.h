// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include <stdbool.h>

// Provide event handling. Events from the system are pushed onto an event
// queue by the handle function. The handle function should be called by the
// main thread, which then does nothing else. The tick function pushes regular
// tick events for cursor blinking and autosave onto the event queue. It should
// be called by a separate thread which does nothing else. (The main body of
// the program should be run on a separate runner thread.)

// TODO: find out what CTRL+ does, e.g. on keyboards where + is unshifted

// A handler manages the event queue.
struct handler;
typedef struct handler handler;

// Declare the event type, avoiding the inclusion of the event header.
typedef int event;

// Make a new handler, based on the given GLFW window, passed as a void pointer
// to prevent the GLFW/OpenGL APIs being exposed.
handler *newHandler(void *w);

// Free up the handler and its data.
void freeHandler(handler *h);

// Handle system events. Call from the main thread. Returns on QUIT.
void handle(handler *h);

// Handle timer events. Call from a separate ticker thread. Doesn't return.
void tick(handler *h);




// Check whether the window has the input focus.
bool focused(handler *h);

// Set a new blink rate.
void setBlinkRate(handler *h, double br);

// Get the next event, possibly with a pause. Fill in the px, py, pt variables.
event getEvent(handler *h, int *px, int *py, char const **pt);

// Add an event from the runner thread, e.g. PASTE or FRAME.
void addEvent(handler *h, event e, int x, int y, char const *t);

// Generate a frame tick event. Call when drawing a frame with vsync.
// oid frameTick(handler *h);

// Generate a paste event. Call when C_V is pressed.
// void pasteEvent(handler *h);

// Copy text on CUT/COPY from the document back to the OS.
void clip(handler *h, char const *text);
