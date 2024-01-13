// The Snipe editor is free and open source. See licence.txt.
#include "unicode.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>

// The same graphics library as the display and handler modules is used to query
// fonts about grapheme clusters.
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// See https://nullprogram.com/blog/2017/10/06/. This variant doesn't assume
// that the input is padded.
extern inline Character getUTF8(char const *s) {
    static const char lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
    };
    static const int masks[] = {0x00, 0x7f, 0x1f, 0x0f, 0x07};
    unsigned char const *b = (unsigned char const *) s;
    int len = lengths[b[0] >> 3];
    int ch = b[0] & masks[len];
    for (int i = 1; i < len; i++) ch = (ch << 6) | (b[i] & 0x3F);
    return (Character) { .code = ch, .length = len };
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

// Check that text is UTF8 valid. Exclude most ASCII control characters. Return
// an error message or null. See
// https://www.w3.org/International/questions/qa-forms-utf-8
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

void utf16to8(wchar_t const *ws, char *s) {
    int n = wcslen(ws);
    int out = 0;
    for (int i = 0; i < n; i++) {
        wchar_t wc = ws[i];
        if (wc < 0x80) {
            s[out++] = (char) wc;
        }
        else if (wc < 0x800) {
            s[out++] = (char) (0xc0 | (wc >> 6));
            s[out++] = (char) (0x80 | (wc & 0x3f));
        }
        else if (wc < 0xd800 || wc >= 0xe000) {
            s[out++] = (char) (0xe0 | (wc >> 12));
            s[out++] = (char) (0x80 | ((wc >> 6) & 0x3f));
            s[out++] = (char) (0x80 | (wc & 0x3f));
        }
        else {
            int ch = 0x10000 + (((wc & 0x3ff) << 10) | (ws[++i] & 0x3ff));
            s[out++] = (char) (0xf0 | (ch >> 18));
            s[out++] = (char) (0x80 | ((ch >> 12) & 0x3f));
            s[out++] = (char) (0x80 | ((ch >> 6) & 0x3f));
            s[out++] = (char) (0x80 | (ch & 0x3f));
        }
    }
    s[out] = '\0';
}

void utf8to16(char const *s, wchar_t *ws) {
    int out = 0;
    for (int i = 0; i < strlen(s); ) {
        Character cp = getUTF8(&s[i]);
        int ch = cp.code;
        i += cp.length;
        if (ch < 0x10000) ws[out++] = (wchar_t) ch;
        else {
            ch = ch - 0x10000;
            ws[out++] = 0xd800 | ((ch >> 10) & 0x3ff);
            ws[out++] = 0xdc00 | (ch & 0x3ff);
        }
    }
    ws[out] = 0;
}

// The unit of text used in "character" motion, backspace etc. is not the byte
// or the code point but the grapheme cluster. The Unicode standard has an
// algorithm for breaking text into grapheme clusters but, as the standard
// says, where text is displayed, what matters more are the fonts and the
// renderer. So, the currently used font (actually a chain of fonts) and the
// allegro ttf addon are used to determine combining characters. This function
// assumes allegro itself has been initialised.
bool combined(int code1, int code2, Font *f) {
    ALLEGRO_FONT *font = (ALLEGRO_FONT *) f;
    return al_get_glyph_advance(font, code1, code2) == 0;
}

// ---------- Testing ----------------------------------------------------------
#ifdef unicodeTest

static void testGetUTF8() {
    char *s = "\xE2\x80\x8C";
    Character cp = getUTF8(s);
    assert(cp.code == 0x200C && cp.length == 3);
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

static void test16() {
    wchar_t w[] = {
        0x1, 0x7f, 0x80, 0xd7ff, 0xd800 | 0x3ef, 0xdcba, 0xe000, 0xffff, 0
    };
    wchar_t x[10];
    char s[20];
    utf16to8(w, s);
    utf8to16(s, x);
    assert(wcslen(w) == wcslen(x));
    for (int i = 0; i < wcslen(w); i++) {
        assert(x[i] == w[i]);
    }
}

int main(int n, char const *args[n]) {
    testGetUTF8();
    testCheck2();
    testCheck3();
    testCheck4();
    test16();
    printf("Unicode module OK\n");
    return 0;
}

#endif
