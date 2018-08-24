// The Snipe editor is free and open source, see licence.txt.
#include "display.h"
#include "document.h"

// A map translates events into actions, using a file in the maps directory.
// It also maps actions to functions, either in the document or the handler.
struct map;
typedef struct map map;

// Declare the event type, avoiding the inclusion of the event header.
typedef int event;

// Create a map from the map file given in the settings.
map *newMap(document *doc, display *dis, bool testing);

// Free up the map.
void freeMap(map *m);

// Redraw the display from the document data.
void redraw(map *m);

// Offer an action both to the document and to the display.
bool dispatch(map *m, event e, int r, int c, char const *t);
