// The Snipe editor is free and open source, see licence.txt.
#include <stddef.h>
#include <stddef.h>

// Read a UTF8 character, and report its length.
int getUTF8(char const *t, int *plength);

// Convert a unicode character into a UTF8 string (of up to 4 bytes plus '\0').
void putUTF8(unsigned int code, char *s);

// Check that text is UTF8 valid. Non-newline ASCII control characters are
// invalid. Assume s[n+1] exists. Return an error message or NULL.
char const *utf8valid(char *s, int n);

// Convert a UTF16 string to a UTF8 string. (Allow twice the number of bytes.)
void utf16to8(wchar_t const *ws, char *s);

// Convert a UTF8 string to a UTF16 string. (Allow twice the number of bytes.)
void utf8to16(char const *s, wchar_t *ws);
