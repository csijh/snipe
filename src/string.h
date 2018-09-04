// The Snipe editor is free and open source, see licence.txt.

// String and UTF8 utilities.
#include "list.h"
#include <string.h>

// Split a string in place at the newlines into a list of lines.
// Assume that the string has been normalized.
strings *splitLines(char *s);

// Split a line in place at the spaces into a list of words.
strings *splitWords(char *s);

// Convert line endings to \n and tabs to spaces, delete trailing spaces and
// trailing blank lines. Add a final newline if necessary, assuming there is
// space for it. Return the adjusted length.
int normalize(char *s);
