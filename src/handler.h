// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include <stdbool.h>

// Provide an event handling service for the display module.
// TODO: find out what CTRL+ does, e.g. on keyboards where + is unshifted
// TODO: find out what scroll events are like with a touchpad.

// An event handler interprets system events as editor events.
struct handler;
typedef struct handler handler;

// Declare the event type, avoiding the inclusion of the event header.
typedef int event;

// Forward reference to the type of map objects, to avoid circular dependencies.
struct map;
typedef struct map map;

// Define the type of the dispatch function (the event callback).
typedef void dispatcher(map *m, event e, int r, int c, char *t);

// Make a new handler. The first argument is the GLFW window, passed as a void
// pointer to prevent the GLFW/OpenGL APIs being exposed. The other two
// arguments are the event dispatch function and the map object to pass to it.
handler *newHandler(void *w, dispatcher *f, map *m);

// Free up the handler and its data.
void freeHandler(handler *h);

// Check whether the window has the input focus.
bool focused(handler *h);

// Set a new blink rate.
void setBlinkRate(handler *h, double br);

// Get the next event, possibly with a pause.
event getRawEvent(handler *h, int *x, int *y, char const **t);

// Generate a frame tick event. Call when drawing a frame with vsync.
void frameTick(handler *h);

// Generate a paste event. Call when C_V is pressed.
void pasteEvent(handler *h);

// Copy text from the document back to the OS.
void clip(handler *h, char const *text);
