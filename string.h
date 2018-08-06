// The Snipe editor is free and open source, see licence.txt.

// String and UTF8 utilities.
#include "list.h"
#include <string.h>

// Split a string in place at the newlines into a list of lines.
// Assume that the string has been normalized.
strings *splitLines(char *s);

// Split a line in place at the spaces into a list of words.
strings *splitWords(char *s);

// Read a UTF8 character, and report its length.
int getUTF8(char const *t, int *plength);

// Convert a unicode character into a UTF8 string (of up to 4 bytes plus '\0').
void putUTF8(unsigned int code, char *s);

// Check that text is UTF8 valid. Non-newline ASCII control characters are
// invalid. Assume s[n+1] exists. Return an error message or NULL.
char const *utf8valid(char *s, int n);

// Convert line endings to \n and tabs to spaces, delete trailing spaces and
// trailing blank lines. Add a final newline if necessary, assuming there is
// space for it. Return the adjusted length.
int normalize(char *s);
