// The Snipe editor is free and open source, see licence.txt.
//#include "list.h"
#include "cursor.h"

// A text object is a flexible array holding the content of a file. It is
// implemented as a gap buffer. For n bytes, positions in the text run from 0
// (before the first byte) to n (after the last byte, and therefore visually
// below the last line). A final newline is maintained. If the text has h lines,
// they are indexed 0 to h-1, but an implicit blank line h is provided for
// display. A text object looks after the positions of line boundaries, and a
// set of cursors. The text positions in the lines and cursors are adjusted at
// each insertion or deletion. Also, a list of style bytes for the text is
// maintained, and its tail is invalidated after each insertion or deletion.
struct text;
typedef struct text text;

// Create an empty text object, or free a text object and its data.
text *newText(void);
void freeText(text *t);

// Get the lines or the cursors or the styles.
ints *getLines(text *t);
cursors *getCursors(text *t);
chars *getStyles(text *t);
ints *getIndents(text *t);

// Return the number of bytes.
int lengthText(text *t);

// Make a copy of a given sequence of n bytes into s.
void getText(text *t, int p, int n, chars *s);

// Insert the string s at position p. Update the array of line end positions and
// the array of cursor positions.
void insertText(text *t, int p, char const *s);

// Insert text at cursor positions
void insertAt(text *t, char const *s);

// Copy text from the selection into the given string. If there are multiple
// selections, gather them separated by newlines.
void gatherText(text *t, chars *s);

// Delete text specified by cursor selections.
void deleteAt(text *t);

// Delete the n bytes starting at position p. Update the array of line endings
// and the array of cursor positions.
void deleteText(text *t, int p, int n);

// Read the given file into a text object. Validate and normalize the text, and
// convert the newlines to nulls. On failure, print a message and return NULL.
// Add the line ending positions to the given list.
text *readText(char const *path);

// Write the text to a given file. Nulls are converted to newlines.
void writeText(text *t, char const *path);
