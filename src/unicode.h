// The Snipe editor is free and open source. See licence.txt.
#include <stddef.h>
#include <stdbool.h>

// The byte length of a UTF-8 sequence.
int ulength(char const *s);

// The Unicode code point of a UTF-8 sequence of the given length.
int ucode(char const *s);

// Convert a unicode character into a UTF8 string (of up to 4 bytes plus '\0').
// Return the length of the string.
int putUTF8(unsigned int code, char *s);

// Check that text is UTF8 valid. Non-newline ASCII control characters are
// invalid. Assume s[n+1] exists. Return an error message or NULL.
char const *utf8valid(char *s, int n);

// Convert a UTF16 string to a UTF8 string. (Allow twice the number of bytes.)
// Return the UTF8 length.
int utf16to8(wchar_t const *ws, char *s);

// Convert a UTF8 string to a UTF16 string. (Allow twice the number of bytes.)
// Return the UTF16 length.
int utf8to16(char const *s, wchar_t *ws);
