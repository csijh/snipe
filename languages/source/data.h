// Snipe language compiler. Free and open source. See licence.txt.
#include "list.h"
#include <stdbool.h>

// Read a binary or text file. If text, add a newline and a null terminator.
char *readFile(char const *path, bool binary);

// Split the text into a newly allocated list of lines, replacing each newline
// in the original text by a null.
char **splitLines(char *text);

// Split a line into the given list of tokens, which may be NULL. Return
// the possibly reallocated list.
char **splitTokens(int row, char *line, char *tokens[]);
