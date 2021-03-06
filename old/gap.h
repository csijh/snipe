// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include "op.h"

// A text object is a flexible byte array holding the content of a file. It is
// implemented as a gap buffer. For n bytes, positions in the text run from 0
// (before the first byte) to n (after the last byte).
struct text;
typedef struct text text;

// Create an empty text object, or free a text object and its data.
text *newText(void);
void freeText(text *t);

// Return the number of bytes.
int lengthText(text *t);

// Make a copy of a given sequence of n bytes into s.
void getText(text *t, int at, int n, chars *s);

// Insert the string s at a given position.
void insertText(text *t, int at, int n, char const s[n]);

// Delete the n bytes starting at a given position.
void deleteText(text *t, int at, int n);

// Change the text according to an edit or adjustment.
void changeText(text *t, op *o);

// Read the given file into a text object. Validate and normalize the text. On
// failure, print a message and return NULL.
text *readText(char const *path);

// Write the text to a given file.
void writeText(text *t, char const *path);
