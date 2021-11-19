// Snipe language compiler. Free and open source. See licence.txt.
#include <stdbool.h>

// A string is a read-only UTF-8 array of bytes, which may contain nulls.
struct string;
typedef struct string string;
typedef unsigned char byte;

// Create a string as a copy of an array of char.
string *newString(int n, char *s);

// Free a string.
void freeString(string *s);

// For a pattern, restrict the length to 127 bytes. If the string contains ..
// then check it is a valid range.
void checkPattern(string *s, int row);

// Find the length of a string.
int length(string *s);

// Get the i'th byte of a string.
byte at(string *s, int i);

// Compare two strings in UTF-8 lexicographic order.
int compare(string const *s1, string const *s2);

// Create a substring of a string, from byte position i to byte position j.
string *substring(string *s, int i, int j);

// Convert escape sequences in a string, in place, into characters. The row
// number is used in error messages.
void unescape(string *s, int row);

// Check if a string is a range pattern such as 0..9. Crash if there isn't a
// single character either side (or the pattern is ..).
bool isRange(string *s, int row);

// Find the from/to code point from a range.
int from(string *s);
int to(string *s);

// Make a new range pattern string.
string *newRange(int from, int to);

// Read a text file as a string. Convert \t or \r to ' '. Ensure final newline.
string *readFile(char const *path);

// Give error message, with printf-style parameters, and exit.
void crash(char const *message, ...);
