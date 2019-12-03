// The Snipe editor is free and open source, see licence.txt.
#include "array.h"
#include <stdbool.h>

// A text object is a flexible byte array holding the content of a file. It is
// implemented as a gap buffer. For n bytes, points (positions) in the text run
// from 0 (at the start) to n (after the last byte). Invariants are preserved to
// maintain the physical integrity of the text, in case of an autosave at any
// time. For insertions, invalid UTF-8 byte sequences, nulls, and carriage
// returns are removed. Insertions and deletions are adjusted to avoid trailing
// spaces on the end of lines. After an insertion or deletion, the end of the
// text is adjusted to remove trailing blank lines or add a final newline.
// Cursors and selections are not tracked by this module, and may be to the
// right of the physical end of a line, or below the last physical line. For
// efficiency, the gap can be moved gradually using moveText, e.g. during a long
// scroll, to avoid a single big move.
struct text;
typedef struct text text;
typedef unsigned int point;

// Create an empty text object, or free a text object and its data.
text *newText(void);
void freeText(text *t);

// Fill a text object from a newly loaded file, discarding any previous content.
// Return false if the buffer contains invalid UTF-8 sequences or nulls.
bool loadText(text *t, chars buffer[]);

// Return the number of bytes.
length lengthText(text *t);

// Make a copy of n characters of text at a given point in s as a substring.
// Returning the possibly resized string.
chars *getText(text *t, point at, length n, chars s[]);

// Insert the given substring. The text, length and position of the substring
// are changed as necessary so that the insertion maintains the invariants. The
// possibly resized string is returned.
chars *insertText(text *t, chars s[]);

// Delete a substring according to its position and length, copying the deleted
// bytes into the substring. The string's position and length are adjusted as
// necessary to maintain the invariants. Return the possibly resized substring.
chars *deleteText(text *t, chars s[]);

// Call this to move the gap in the gap buffer. For efficiency only.
void moveText(text *t, point at);
