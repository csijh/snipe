// The Snipe editor is free and open source, see licence.txt.
#include "edit.h"
#include <stdbool.h>

// A text object holds the UTF-8 content of a file, its cursors, and which
// cursor is current. For n bytes, there are n+1 positions in the text, running
// from 0 (at the start) to n (after the final newline). The text never contains
// invalid UTF-8 sequences or control characters '\0' to '\7' or carriage
// returns or a missing final newline. It does not contain trailing spaces at
// the ends of lines, or trailing blank lines, other than where needed to
// support cursors. On saving, these trailers are omitted. The array of cursors
// is stored in order of their positions (start of selections) in the text.
struct text;
typedef struct text text;

// Create an empty text object, or free a text object and its data.
text *newText(void);
void freeText(text *t);

// Return the number of bytes. The maximum length is INT_MAX, so that int can be
// used for positions, and for positive or negative relative positions.
int lengthText(text *t);

// Fill a text object from a newly loaded file, discarding any previous content.
// Return false if the buffer contains invalid UTF-8 sequences or control
// characters '\0 to '\7 (because it is probably binary and shouldn't be loaded)
// or the text is too long.
bool loadText(text *t, int n, char *buffer);

// Copy the text out into a buffer, removing any trailers.
char *saveText(text *t, char *buffer);

// Insert s at a given position. Cursors are adjusted. Any cursor end at the
// given position is moved to the end of the inserted text.
void insertText(text *t, int at, int n, char s[n]);

// Delete text in a range (with to < from allowed). Any cursor end within the
// range, or at the left end, is moved to the right end before the deletion.
void deleteText(text *t, int from, int to);

// Create a new cursor at the given position. It is inserted in order into the
// array of cursors and becomes current.
void addCursor(text *t, int at);

// Remove the current cursor. The next cursor becomes current, or the previous
// one if the last cursor is removed.
void cutCursor(text *t);

// Get the number of cursors.
void countCursors(text *t);

// Make the c'th cursor current.
void setCursor(text *t, int c);

// Get the current cursor index, the start or end of its selection, the right or
// left end of its selection, or its remembered column.
int cursorIndex(text *t);
int cursorFrom(text *t);
int cursorTo(text *t);
int cursorLeft(text *t);
int cursorColumn(txt *t);

// Collapse overlapping or ambiguously touching cursors. Call at the end of
// every user action, other than a drag.
void collapseCursors(text *t);

// Undo the most recent user action, taking normal or small steps. Small steps
// correspond to 'tree-based undo', and also unpick combined typed characters.
void undoText(text *t, bool small);

// Redo the most recent undone user action, taking normal or small steps.
void redoText(text *t, bool small);

// After each edit is executed, the range of text which has been changed can be
// used for incremental changes in other modules, and reset afterwards.
int startChanged(text *t);
int endChanged(text *t);
void resetChanged(text *t);

// Make a copy in s of n characters of text at a given position.
void getText(text *t, position at, int n, char *s);
