// Snipe unicode support. Free and open source, see licence.txt.
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

// The success and failure states of the UTF-8 byte state machine, and the
// unicode replacement code point for all invalid UTF-8 sequences.
enum { UTF8_ACCEPT = 0, UTF8_REJECT = 12, UBAD = 0xFFFD };

// The state machine tables are adapted from
// http://bjoern.hoehrmann.de/utf-8/decoder/dfa/, copyright (c) 2008-2010 Bjoern
// Hoehrmann <bjoern@hoehrmann.de>.
typedef unsigned char byte;

// Table to look up a byte and get its type. Public to allow function inlining.
const byte utf8ByteTable[] = {
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,
};

// State machine to look up a state and byte-type and get the next state.
// Public to allow function inlining.
const byte utf8StateTable[] = {
     0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
    12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
    12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
    12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
    12,36,12,12,12,12,12,12,12,12,12,12,
};

// Process one UTF-8 byte, using the state machine tables.
extern inline int decodeByte(int state, byte b, int *codep) {
    int type = utf8ByteTable[b];
    *codep = (state != UTF8_ACCEPT) ?
        (b & 0x3fu) | (*codep << 6) :
        (0xff >> type) & b;
    state = utf8StateTable[state + type];
    return state;
}

// Check if a UTF-8 string is well formed.
bool uvalid(int n, char s[n]) {
    int state = UTF8_ACCEPT, code = 0;
    for (int i = 0; i < n; i++) state = decodeByte(state, s[i], &code);
    return state == UTF8_ACCEPT;
}

extern inline character getCode(const char *s) {
    character ch;
    ch.length = 0;
    int state = UTF8_ACCEPT;
    state = decodeByte(state, s[ch.length++], &ch.code);
    while (state != UTF8_ACCEPT && state != UTF8_REJECT) {
        state = decodeByte(state, s[ch.length++], &ch.code);
    }
    if (state == UTF8_REJECT) ch.code = UBAD;
    return ch;
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

#ifdef TESTunicode
// ----------------------------------------------------------------------------

// Test a UTF-8 byte sequence.
bool ok(char const *s, int length, int code) {
    character ch = getCode(s);
    return (ch.length == length && ch.code == code);
}

// Test all the boundary cases of reading a code point from UTF-8.
void codeTest() {
    assert(ok("\0", 1, 0));
    assert(ok("\x7F", 1, 0x7F));
    assert(ok("\xC0\xBF", 1, UBAD));             // overlong
    assert(ok("\xC1\xBF", 1, UBAD));             // overlong
    assert(ok("\xC2\x7F", 2, UBAD));             // bad 2nd byte
    assert(ok("\xC2\x80", 2, 0x80));             // start of 8-bit data
    assert(ok("\xC2\xBF", 2, 0xBF));             // full second byte
    assert(ok("\xC2\xC0", 2, UBAD));             // bad 2nd byte
    assert(ok("\xC3\xBF", 2, 0xFF));             // end of 8-bit data
    assert(ok("\xDF\x80", 2, 0x7C0));            // start of 11-bit data
    assert(ok("\xDF\xBF", 2, 0x7FF));            // end of 11-bit data
    assert(ok("\xE0\x9F\xBF", 2, UBAD));         // overlong
    assert(ok("\xE0\xA0\x80", 3, 0x800));        // start of 12-bit data
    assert(ok("\xE0\xBF\xBF", 3, 0xFFF));        // end of 12-bit data
    assert(ok("\xE8\x80\x80", 3, 0x8000));       // start of 13-bit data
    assert(ok("\xED\xA0\x80", 2, UBAD));         // UTF-16 surrogates
    assert(ok("\xED\xBF\xBF", 2, UBAD));         // UTF-16 surrogates
    assert(ok("\xEF\xBF\xBF", 3, 0xFFFF));       // end of 16-bit data
    assert(ok("\xF0\x8F\xBF\xBF", 2, UBAD));     // overlong
    assert(ok("\xF0\x90\x80\x80", 4, 0x10000));  // start of 17-bit data
    assert(ok("\xF4\x8F\xBF\xBF", 4, 0x10FFFF)); // limit 1114111
    assert(ok("\xF4\x90\x80\x80", 2, UBAD));     // > limit
}

int main() {
    codeTest();
    printf("Unicode module OK\n");
    return 0;
}

#endif
