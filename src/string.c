// The Snipe editor is free and open source, see licence.txt.
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// For utf8valid, see
// https://www.w3.org/International/questions/qa-forms-utf-8

// Add a string to a list.
static void add(strings *xs, char *s) {
    int n = length(xs);
    resize(xs, n + 1);
    S(xs)[n] = s;
}

strings *splitLines(char *s) {
    strings *lines = newStrings();
    char *line = s;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != '\n') continue;
        s[i] = '\0';
        add(lines, line);
        line = &s[i+1];
    }
    return lines;
}

strings *splitWords(char *s) {
    strings *words = newStrings();
    char *word = s;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != ' ') continue;
        s[i] = '\0';
        add(words, word);
        while (s[i+1] == ' ') i++;
        word = &s[i+1];
    }
    if (word[0] != '\0') add(words, word);
    return words;
}

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

int normalize(char *s) {
    int out = 0;
    for (int in = 0; s[in] != '\0'; in++) {
        byte b = s[in];
        if (b == '\t') { s[out++] = ' '; continue; }
        if (b != '\r' && b != '\n') { s[out++] = b; continue; }
        if (b == '\r' && s[in + 1] == '\n') in++;
        while (out >= 1 && s[out - 1] == ' ') out--;
        s[out++] = '\n';
    }
    while (out >= 1 && s[out-1] == ' ') out--;
    while (out >= 1 && s[out-1] == '\n') out--;
    s[out++] = '\n';
    s[out] = '\0';
    return out;
}

#ifdef test_string

static void testSplitLines() {
    char s[] = "a\nbb\nccc\n";
    strings *lines = splitLines(s);
    assert(length(lines) == 3);
    assert(strcmp(S(lines)[0], "a") == 0);
    assert(strcmp(S(lines)[1], "bb") == 0);
    assert(strcmp(S(lines)[2], "ccc") == 0);
    freeList(lines);
}

static void testSplitWords() {
    char s[] = "a bb    ccc";
    strings *words = splitWords(s);
    assert(length(words) == 3);
    assert(strcmp(S(words)[0], "a") == 0);
    assert(strcmp(S(words)[1], "bb") == 0);
    assert(strcmp(S(words)[2], "ccc") == 0);
    freeList(words);
}

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

// Run a test on the normalize function.
static bool norm(char *in, char *out) {
    char *t = malloc(100);
    strcpy(t, in);
    normalize(t);
    bool b = strcmp(t, out) == 0;
    free(t);
    return b;
}

// Test the normalize function.
static void testNorm() {
    // Convert all line endings (\n, \r, \r\n) to \n
    assert(norm("v\n", "v\n"));
    assert(norm("v\r", "v\n"));
    assert(norm("v\r\n", "v\n"));
    // Variations
    assert(norm("v\rw\n", "v\nw\n"));
    assert(norm("v\r\nw\n", "v\nw\n"));
    assert(norm("v\n\nw\n", "v\n\nw\n"));
    // Remove trailing spaces
    assert(norm("v   \nw\n", "v\nw\n"));
    assert(norm("v\nw   \n", "v\nw\n"));
    // Remove trailing blank lines
    assert(norm("v\n\n", "v\n"));
    assert(norm("v\n\n\n\n", "v\n"));
    // Add final newline
    assert(norm("v", "v\n"));
    assert(norm("v\nw", "v\nw\n"));
    assert(norm("v   ", "v\n"));
}

int main(int n, char const *args[n]) {
    testSplitLines();
    testSplitWords();
    testGetUTF8();
    testCheck2();
    testCheck3();
    testCheck4();
    testNorm();
    printf("String module OK\n");
    return 0;
}

#endif
