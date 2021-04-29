// Snipe language compiler. Free and open source. See licence.txt.
#include <stdbool.h>

// Lists of strings, implemented as flexible arrays.
struct strings;
typedef struct strings strings;

// Give error message, with line number or 0, and extra info or "", and exit.
void crash(char const *message, ...);

// Read a binary or text file. If text, add a newline and a null terminator.
char *readFile(char const *path, bool binary);

// Split the text into a list of lines, replacing newlines in the text by nulls.
void splitLines(char *text, strings *ss);

// Split a line into the given list of tokens, replacing spaces by nulls.
void splitTokens(int row, char *line, strings *tokens);

// Create a list of strings.
strings *newStrings();

// Free the data in a list of strings, but not the descriptor.
void freeStrings(strings *ss);

// Find the length of a list of strings.
int countStrings(strings *ss);

// Set the length of a list of strings to zero.
void clearStrings(strings *ss);

// Get the i'th string in the list.
char *getString(strings *ss, int i);

// Set the i'th string in the list.
void setString(strings *ss, int i, char *s);

// Add a string to a list.
int addString(strings *ss, char *s);
