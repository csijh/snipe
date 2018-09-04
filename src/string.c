// The Snipe editor is free and open source, see licence.txt.
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef unsigned char byte;

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
    if (out > 0) s[out++] = '\n';
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
