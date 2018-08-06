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

// Make a new handler, based on the given GLFW window, passed as a void pointer
// to prevent the GLFW/OpenGL APIs being exposed.
handler *newHandler(void *w);

// Free up the handler and its data.
void freeHandler(handler *h);

// Set a new blink rate.
void setBlinkRate(handler *h, double br);

// Get the next event, possibly with a pause.
event getRawEvent(handler *h, int *x, int *y, char **t);

// Generate a frame tick event. Call when drawing a frame with vsync.
void frameTick(handler *h);
