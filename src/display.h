// The Snipe editor is free and open source, see licence.txt.

// Create and manage the main window. Deal with converting text to syntax
// highlighted images, scrolling, pixel coordinates. Delegate event handling to
// a handler object.
//
// TODO: add overlays.
// TODO: handle multiple fonts and multiple pages and UTF8.
#include "handler.h"

// The editor's graphical user interface.
struct display;
typedef struct display display;

// Declare the action type, avoiding the inclusion of the action header.
typedef int action;

// The type of a run function to be executed on the runner thread.
typedef void *runFunction(void *p);

// Create a display, with user preference settings.
display *newDisplay(char const *path);

// Free up the display object and its contents.
void freeDisplay(display *d);

// Find the number of rows (for PAGEUP/DOWN).
int pageHeight(display *d);

// Set up handler and ticker threads, and execute the provided run function on
// a separate runner thread, passing the given pointer.
void startGraphics(display *d, runFunction *run, void *p);

// Set the window title according to the current file path.
void setTitle(display *d, char const *path);

// Set the top displayed row, as a target for smooth scrolling.
void setScrollTarget(display *d, int row);

// The first row of the document that is visible on screen.
int firstRow(display *d);

// The last row of the document that is visible on screen.
int lastRow(display *d);

// Get the next event. For a mouse click, (r,c) are set to the character
// coordinates in the document. For text input, t is set to the text, which is
// valid only until the following event.
event getEvent(display *d, int *r, int *c, char const **t);

// Create an image of a line from its row number, text, and style info.
void drawLine(display *d, int row, int n, char *line, char *styles);

// Make recent changes appear on screen, with a vertical sync delay.
void showFrame(display *d);

// Carry out the given action, if relevant.
void actOnDisplay(display *d, action a, char const *s);
