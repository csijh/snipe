// Snipe unicode support. Free and open source, see licence.txt.
#include <stdbool.h>

// Provide iteration through code points of UTF-8 text, with grapheme
// boundaries. The lookup tables are inserted into this module automatically by
// unigen.c, from Unicode data files. The current version of the Unicode
// standard supported is 12.0.0.

// The unicode replacement code point for all invalid UTF-8 sequences.
enum { UBAD = 0xFFFD };

// Check if string is UTF-8 valid.
bool uvalid(int n, char s[n]);

// The code and byte-length of a UTF-8 code point, whether the character is a
// joiner to be printed on top of the previous character, plus grapheme boundary
// info for internal use.
struct codePoint { int code; unsigned char length, joiner, grapheme; };
typedef struct codePoint const codePoint;

// Get an initial codePoint suitable for iterating through a string.
codePoint initialCodePoint();

// Get the next code point. The first call should be on a grapheme boundary,
// after which grapheme boundaries will be tracked.
void nextCode(codePoint *cp, const char *s);
