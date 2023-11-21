// Snipe Unicode support. Free and open source, see licence.txt.
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

// The Unicode value for invalid UTF-8 sequences.
enum { UBAD = 0xFFFD };

// Get a Unicode character from UTF8 text, with a full check for validity,
// returning UBAD for failure (and taking into account that char may be signed
// or unsigned).
character getUTF8(const char *s) {
    unsigned int code, length;
    if ((s[0] & 0x80) == 0) { code = s[0]; length = 1; }
    else if ((s[0] & 0xE0) == 0xC0) {
        if ((s[1] & 0xC0) != 0x80) { code=UBAD; length=2; }
        else {
            length = 2;
            code = ((s[0] & 0x1F) << 6);
            code += (s[1] & 0x3F);
            if (code <= 0x7F) code = UBAD;
        }
    }
    else if ((s[0] & 0xF0) == 0xE0) {
        length = 3;
        if ((s[1] & 0xC0) != 0x80) code=UBAD;
        else if ((s[2] & 0xC0) != 0x80) code=UBAD;
        else {
            code = ((s[0] & 0x1F) << 12);
            code += ((s[1] & 0x3F) << 6);
            code += ((s[2] & 0x3F));
            if (code <= 0x7FF) code = UBAD;
            if (0xD800 <= code && code <= 0xDFFF) code = UBAD;
        }
    }
    else if ((s[0] & 0xF8) == 0xF0) {
        length = 4;
        if ((s[1] & 0xC0) != 0x80) code=UBAD;
        else if ((s[2] & 0xC0) != 0x80) code=UBAD;
        else if ((s[3] & 0xC0) != 0x80) code=UBAD;
        else {
            code = ((s[0] & 0x0F) << 18);
            code += ((s[1] & 0x3F) << 12);
            code += ((s[2] & 0x3F) << 6);
            code += ((s[3] & 0x3F));
            if (code <= 0xFFFF) code = UBAD;
            if (code > 0x10FFFF) code = UBAD;
        }
    }
    else { code = UBAD; length = 1; }
    return (character) { .code = code, .length = length };
}

bool uvalid(int n, char s[n]) {
    int i;
    for (i = 0; i < n; ) {
        character ch = getUTF8(&s[i]);
        if (ch.code == UBAD) return false;
        i += ch.length;
    }
    return (i == n);
}

void putUTF8(unsigned int code, char *s) {
    if (code < 0x7f) {
        s[0] = code;
        s[1] = '\0';
    } else if (code < 0x7ff) {
        s[0] = 0xC0 | (code >> 6);
        s[1] = 0x80 | (code & 0x3F);
        s[2] = '\0';
    } else if (code < 0xffff) {
        s[0] = 0xE0 | (code >> 12);
        s[1] = 0x80 | ((code >> 6) & 0x3F);
        s[2] = 0x80 | (code & 0x3F);
        s[3] = '\0';
    } else if (code <= 0x10FFFF) {
        s[0] = 0xF0 | (code >> 18);
        s[1] = 0x80 | ((code >> 12) & 0x3F);
        s[2] = 0x80 | ((code >> 6) & 0x3F);
        s[3] = 0x80 | (code & 0x3F);
        s[4] = '\0';
    } else {
        s[0] = '\0';
    }
}

void crash(char const *fmt, ...) {
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

void try(bool ok, char const *fmt, ...) {
    if (ok) return;
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

// ----------------------------------------------------------------------------
#ifdef TESTunicode

// Test a UTF-8 byte sequence.
bool ok(char const *s, int length, int code) {
    character ch = getUTF8(s);
    return (ch.length == length && ch.code == code);
}

// Test all the boundary cases of reading a code point from UTF-8.
void codeTest() {
    assert(ok("\0", 1, 0));
    assert(ok("\x7F", 1, 0x7F));
    assert(ok("\xBF", 1, UBAD));                 // bad first byte
    assert(ok("\x80", 1, UBAD));                 // bad first byte
    assert(ok("\xC0\xBF", 2, UBAD));             // overlong
    assert(ok("\xC1\xBF", 2, UBAD));             // overlong
    assert(ok("\xC2\x7F", 2, UBAD));             // bad 2nd byte
    assert(ok("\xC2\x80", 2, 0x80));             // start of 8-bit data
    assert(ok("\xC2\xBF", 2, 0xBF));             // full second byte
    assert(ok("\xC2\xC0", 2, UBAD));             // bad 2nd byte
    assert(ok("\xC3\xBF", 2, 0xFF));             // end of 8-bit data
    assert(ok("\xDF\x80", 2, 0x7C0));            // start of 11-bit data
    assert(ok("\xDF\xBF", 2, 0x7FF));            // end of 11-bit data
    assert(ok("\xDF\xC0", 2, UBAD));             // bad 2nd byte
    assert(ok("\xE0\x7F\x7F", 3, UBAD));         // bad 2nd byte of 3
    assert(ok("\xE0\x9F\x7F", 3, UBAD));         // bad 3nd byte
    assert(ok("\xE0\x9F\xBF", 3, UBAD));         // overlong
    assert(ok("\xE0\xA0\x80", 3, 0x800));        // start of 12-bit data
    assert(ok("\xE0\xA0\x7F", 3, UBAD));         // bad 3nd byte
    assert(ok("\xE0\xBF\xBF", 3, 0xFFF));        // end of 12-bit data
    assert(ok("\xE8\x80\x80", 3, 0x8000));       // start of 13-bit data
    assert(ok("\xE8\xC0\x80", 3, UBAD));         // bad 2nd byte
    assert(ok("\xE8\xA0\x7F", 3, UBAD));         // bad 3rd byte
    assert(ok("\xED\xA0\x80", 3, UBAD));         // UTF-16 surrogates
    assert(ok("\xED\xBF\xBF", 3, UBAD));         // UTF-16 surrogates
    assert(ok("\xEF\xBF\xBF", 3, 0xFFFF));       // end of 16-bit data
    assert(ok("\xEF\xBF\xC0", 3, UBAD));         // bad 3rd byte
    assert(ok("\xF0\x7F\xBF\xBF", 4, UBAD));     // bad 2nd byte
    assert(ok("\xF0\x8F\xBF\xBF", 4, UBAD));     // overlong
    assert(ok("\xF0\x90\x7F\x80", 4, UBAD));     // bad 3rd byte
    assert(ok("\xF0\x90\x80\x7F", 4, UBAD));     // bad 4th byte
    assert(ok("\xF0\x90\x80\x80", 4, 0x10000));  // start of 17-bit data
    assert(ok("\xF4\x8F\xBF\xBF", 4, 0x10FFFF)); // limit 1114111
    assert(ok("\xF4\x8F\xBF\xC0", 4, UBAD));     // bad 4th byte
    assert(ok("\xF4\x8F\xC0\xBF", 4, UBAD));     // bad 3rd byte
    assert(ok("\xF4\xC0\xBF\xBF", 4, UBAD));     // bad 2nd byte
    assert(ok("\xF4\x90\x80\x80", 4, UBAD));     // > limit
    assert(ok("\xF8", 1, UBAD));                 // bad first byte
    assert(ok("\xFF", 1, UBAD));                 // bad first byte
}

int main() {
    codeTest();
    printf("Unicode module OK\n");
    return 0;
}

#endif
