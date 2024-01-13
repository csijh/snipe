// The Snipe editor is free and open source. See licence.txt.
#include <stddef.h>
#include <stdbool.h>

// The Unicode code point and byte length of a UTF-8 character sequence.
struct character { int code, length; };
typedef struct character Character;

// A graphics-library-independent type for a font.
typedef struct font Font;

// Read a Unicode code point and length from UTF8.
Character getUTF8(char const *s);

// Convert a unicode character into a UTF8 string (of up to 4 bytes plus '\0').
void putUTF8(unsigned int code, char *s);

// Check that text is UTF8 valid. Non-newline ASCII control characters are
// invalid. Assume s[n+1] exists. Return an error message or NULL.
char const *utf8valid(char *s, int n);

// Convert a UTF16 string to a UTF8 string. (Allow twice the number of bytes.)
void utf16to8(wchar_t const *ws, char *s);

// Convert a UTF8 string to a UTF16 string. (Allow twice the number of bytes.)
void utf8to16(char const *s, wchar_t *ws);

// Given two Unicode code points, use the given font to check whether the second
// is a combiner, so that it continues the same grapheme cluster.
bool combined(int code1, int code2, Font *font);
