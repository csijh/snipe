// The Snipe editor is free and open source, see licence.txt.
#include "edit.h"
#include <stdbool.h>

// A text object is a flexible byte array holding the content of a file. It is
// implemented as a gap buffer, with the gap maintained at the current cursor
// position. For n bytes, positions in the text run from 0 (at the start) to n
// (after the final newline). The text never contains invalid UTF-8 sequences or
// nulls or returns.

// After each edit is executed, the fixText() function should be called
// repeatedly to obtain further edits needed to repair the text. These repairs
// ensure that, in between user actions, no line has trailing spaces, there are
// no trailing blank lines, and there is no missing final newline. Thus if
// autosave is done at any time, a correct text file is written.

// Selections and multiple cursors are not tracked by this module, only the
// current cursor position. A cursor may be to the right of the physical end of
// a line, or below the last physical line.
struct text;
typedef struct text text;
typedef unsigned int position;

// Create an empty text object, or free a text object and its data.
text *newText(void);
void freeText(text *t);

// Fill a text object from a newly loaded file, discarding any previous content.
// Return false if the buffer contains invalid UTF-8 sequences or nulls (because
// it is probably binary and shouldn't be loaded).
bool loadText(text *t, int n, char *buffer);

// Return the number of bytes.
int lengthText(text *t);

// Make a copy in s of n characters of text at a given position.
void getText(text *t, position at, int n, char *s);

// Update the text according to a requested edit.
void editText(text *t, edit *e);

// Fill in the next edit needed to repair the text. Return false if none.
bool fixText(text *t, edit *e);
