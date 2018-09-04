// The Snipe editor is free and open source, see licence.txt.
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// For utf8valid, see
// https://www.w3.org/International/questions/qa-forms-utf-8

extern inline int getUTF8(char const *t, int *plength) {
    int ch = t[0], len = 1;
    if ((ch & 0x80) == 0) { *plength = len; return ch; }
    else if ((ch & 0xE0) == 0xC0) { len = 2; ch = ch & 0x3F; }
    else if ((ch & 0xF0) == 0xE0) { len = 3; ch = ch & 0x1F; }
    else if ((ch & 0xF8) == 0xF0) { len = 4; ch = ch & 0x0F; }
    for (int i = 1; i < len; i++) ch = (ch << 6) | (t[i] & 0x3F);
    *plength = len;
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

typedef unsigned char byte;

// Check that a, b form a valid character code (8 to 11 bits)
static inline bool check2(byte a, byte b) {
    return ((0xC2 <= a && a <= 0xDF) && (0x80 <= b && b <= 0xBF));
}

// Check that a, b, c are valid (12..16 bits) excluding surrogates
static inline bool check3(byte a, byte b, byte c) {
    if (a == 0xE0) {
        if ((0xA0 <= b && b <= 0xBF) && (0x80 <= c && c <= 0xBF)) return true;
    }
    else if ((0xE1 <= a && a <= 0xEC) || a == 0xEE || a == 0xEF) {
        if ((0x80 <= b && b <= 0xBF) && (0x80 <= c && c <= 0xBF)) return true;
    }
    else if (a == 0xED) {
        if ((0x80 <= b && b <= 0x9F) && (0x80 <= c && c <= 0xBF)) return true;
    }
    return false;
}

// Check that a, b, c, d are valid (17..21 bits up to 1114111)
static inline bool check4(byte a, byte b, byte c, byte d) {
    if (a == 0xF0) {
        if ((0x90 <= b && b <= 0xBF) &&
        (0x80 <= c && c <= 0xBF) &&
        (0x80 <= d && d <= 0xBF)) return true;
    }
    else if (0xF1 <= a && a <= 0xF3) {
        if ((0x80 <= b && b <= 0xBF) &&
        (0x80 <= c && c <= 0xBF) &&
        (0x80 <= d && d <= 0xBF)) return true;
    }
    else if (a == 0xF4) {
        if ((0x80 <= b && b <= 0x8F) &&
        (0x80 <= c && c <= 0xBF) &&
        (0x80 <= d && d <= 0xBF)) return true;
    }
    return false;
}

// Check that text is UTF8 valid. Exclude most ASCII control characters.
// Return an error message or null.
char const *utf8valid(char *s, int length) {
    byte a, b, c, d;
    for (int i = 0; i < length; i++) {
        a = s[i];
        if (' ' <= a && a <= '~') continue;
        if (a == '\r' || a == '\n') continue;
        if (a == '\t') continue;
        if (a == '\0') return "has null characters";
        if (a < 0x80) return "has control characters";
        if (i >= length - 1) return "has invalid UTF-8 text";
        b = s[++i];
        if (check2(a, b)) continue;
        if (i >= length - 1) return "has invalid UTF-8 text";
        c = s[++i];
        if (check3(a, b, c)) continue;
        if (i >= length - 1) return "has invalid UTF-8 text";
        d = s[++i];
        if (check4(a, b, c, d)) continue;
        return "has invalid UTF-8 text";
    }
    return NULL;
}

int length16(int n, wchar_t s[n]) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        wchar_t wc = s[i];
        if (wc < 0x7f) count++;
        else if (wc < 0x7ff) count += 2;
        else if (wc < 0xd800) count += 3;
        else if (wc < 0xdc00) {
            unsigned int code = (wc & 0x3ff) << 10;
            code = code | (s[i++] & 0x3ff);
            code += 0x10000;
            char t[5];
            putUTF8(code, t);
            count += strlen(t);
        }
        else count += 4;
    }
    return count;
}

#ifdef test_unicode

static void testGetUTF8() {
    char *s = "\xE2\x80\x8C";
    int len;
    assert(getUTF8(s, &len) == 0x200C);
    assert(len == 3);
}

static void testCheck2() {
    assert(check2(0xC2, 0x80));   // 8 bits
    assert(check2(0xC2, 0xBF));
    assert(check2(0xDF, 0x80));   // 11 bits
    assert(check2(0xDF, 0xBF));
    assert(! check2(0xC0, 0xBF)); // < 8 bits
    assert(! check2(0xC1, 0xBF));
    assert(! check2(0xC2, 0x7F)); // bad 2nd byte
    assert(! check2(0xC2, 0xC0));
    assert(! check2(0xE0, 0xBF)); // > 11 bits
}

static void testCheck3() {
    assert(check3(0xE0, 0xA0, 0x80));   // 12 bits
    assert(check3(0xE0, 0xBF, 0xBF));
    assert(check3(0xE8, 0x80, 0x80));   // 15 bits
    assert(check3(0xEF, 0xBF, 0xBF));
    assert(! check3(0xE0, 0x9F, 0xBF)); // < 12 bits
    assert(! check3(0xED, 0xA0, 0x80)); // UTF-16 surrogates
    assert(! check3(0xED, 0xBF, 0xBF)); // UTF-16 surrogates
    assert(! check3(0xF0, 0x80, 0x80)); // > 15 bits
}

static void testCheck4() {
    assert(check4(0xF0, 0x90, 0x80, 0x80));   // 16 bits
    assert(check4(0xF4, 0x8F, 0xBF, 0xBF));   // limit 1114111
    assert(! check4(0xF0, 0x8F, 0xBF, 0xBF)); // < 16 bits
    assert(! check4(0xF4, 0x90, 0x80, 0x80)); // > limit
}

int main(int n, char const *args[n]) {
    testGetUTF8();
    testCheck2();
    testCheck3();
    testCheck4();
    printf("Unicode module OK\n");
    return 0;
}

#endif
