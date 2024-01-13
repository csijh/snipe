// The Snipe editor is free and open source. See licence.txt.
#include "event.h"
#include <stdbool.h>

// Provide event handling, on behalf of the display module.
struct handler;
typedef struct handler handler;

// Create an event handler based on the given window.
handler *newHandler(void *window);

// Release an event handler.
void freeHandler(handler *h);

// Get the next event from the graphics library. Provide its extra info, if any.
event getNextEvent(handler *h);
char *getEventText(handler *h);
int getEventX(handler *h);
int getEventY(handler *h);
