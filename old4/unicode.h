// Snipe Unicode support. Free and open source, see licence.txt.
#include <stdbool.h>

// The Unicode code point and byte length of a UTF-8 character sequence.
struct character { int code, length; };
typedef struct character character;

// Check if text s of length n is UTF-8 valid.
bool uvalid(int n, char s[n]);

// Get the next character from the given text.
character getUTF8(const char *s);

// Write a Unicode code point to the given character array.
void putUTF8(unsigned int code, char *s);

// Crash the program with an error message, in printf style.
void crash(char const *fmt, ...);

// Crash the program if the first argument is false.
void try(bool ok, char const *fmt, ...);
