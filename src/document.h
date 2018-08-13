// The Snipe editor is free and open source, see licence.txt.
#include "text.h"
#include <stdbool.h>

// A document holds the contents of a file or folder in memory, with all the
// information needed to edit it or display it.
struct document;
typedef struct document document;

// Define the action type, without including the action header.
typedef int action;

// Create a new document containing a given file or folder.
document *newDocument(char const *path);

// Free up a document object and its contents.
void freeDocument(document *d);

// Get the path to the document's file or folder.
char const *getPath(document *d);

// Check whether the document is a directory.
bool isDirectory(document *d);

// Get the number of lines.
int getHeight(document *d);

// Get the number of bytes in a given line (excluding the newline).
int getWidth(document *d, int row);

// Get the scroll target row.
int getScrollTarget(document *d);

// Get a given line as a character list, valid until the next call.
chars *getLine(document *d, int row);

// Get the style bytes for a line, after scanning, valid until the next call.
chars *getStyle(document *d, int row);

// Apply selection and caret information to the style bytes for a line.
void addCursorFlags(document *d, int row, int n, chars *styles);

// Set row/col/text data for the next event. The text is only valid until the
// following event.
void setData(document *d, int r, int c, char *t);

// Carry out an action on the document.
void actOnDocument(document *d, action a);
