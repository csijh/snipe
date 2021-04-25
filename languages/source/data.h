// Snipe language compiler. Free and open source. See licence.txt.
#include "list.h"

// Read a binary or text file. If text, add a newline and a null terminator.
char *readFile(char const *path, bool binary);

// Split the text into a list of lines, replacing each newline by a null.
char **splitLines(char *text);

// Split a line into a list of tokens.
void splitTokens(language *lang, int row, char *line, char *tokens[]);
