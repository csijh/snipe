// The Snipe editor is free and open source, see licence.txt.

// A text object is a flexible byte array holding the content of a file. It is
// implemented as a gap buffer. For n bytes, points (positions) in the text run
// from 0 (before the first byte) to n (after the last byte).
struct text;
typedef struct text text;
typedef unsigned int point;
typedef unsigned int length;

// Create an empty text object, or free a text object and its data.
text *newText(void);
void freeText(text *t);

// Return the number of bytes.
length lengthText(text *t);

// Get access to a given sequence of n bytes, valid only until the next change.
// A terminating null byte is included.
char const *getText(text *t, point at, length n);

// Insert the string s at a given position.
void insertText(text *t, point at, length n, char const s[n]);

// Like insert, but with cleaning up of the string.
void pasteText(text *t, point at, length n, char const s[n]);

// Delete the n bytes starting at a given position.
void deleteText(text *t, point at, length n);

// Apply an edit to the text.
void editText(edit e);
