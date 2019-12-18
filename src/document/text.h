// The Snipe editor is free and open source, see licence.txt.
#include "edit.h"
#include <stdbool.h>

// A text object holds the UTF-8 content of a file, its current cursor, extra
// cursors, and selections. For n bytes, there are n+1 positions in the text,
// running from 0 (at the start) to n (after the final newline).

// The text never contains invalid UTF-8 sequences or nulls or returns or a
// missing final newline. It does not contain lines with trailing spaces, or
// trailing blank lines, other than where needed to support cursor or selector
// positions. On saving, these trailers are omitted.

// After each edit is executed, the range of text which has been changed can be
// used for incremental changes in other modules.

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

// Return the current position.
int atText(text *t);

// Return the number of cursors.
int cursorsText(text *t);

// Return the position of the i'th cursor.
position cursorText(text *t, int i);

// Return the position of the i'th selector, or -1.
position selectorText(text *t, int i);

// Make a copy in s of n characters of text at a given position.
void getText(text *t, position at, int n, char *s);

// Update the text according to a requested edit.
void editText(text *t, edit *e);

// TODO: insert, delete, add(at), cancel(to), select(to), deselect(from),
// move(to).
