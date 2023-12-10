// Snipe editor. Free and open source, see licence.txt.
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// Sentinel positions in the text.
enum { SentinelB = 0, SentinelE = -1 };

// Return the most recent opener.
int topOpener(int *active) {
    int n = length(active);
    if (n == 0) return SentinelB;
    return active[n-1];
}

// Add a new opener before the cursor. Return the (negative) paired closer
// after the cursor, so the pair can be highlighted as matched or mismatched.
int pushOpener(int *active, int opener) {
    int n = length(active);
    int h = high(active);
    int m = max(active);
    adjust(active, +1);
    active[n] = opener;
    if (h <= m - n - 1) return active[m - n - 1];
    else return SentinelE;
}

// Remove the top opener before the cursor. Return the (negative) paired closer
// after the cursor, which now needs to be marked as unmatched.
int popOpener(int *active) {
    adjust(active, -1);
    int n = length(active);
    int h = high(active);
    int m = max(active);
    if (h <= m - n - 1) return active[m - n - 1];
    else return SentinelE;
}

// On pairing two brackets, remember the opener.
void saveOpener(int *paired, int opener) {
    int n = length(paired);
    adjust(paired, +1);
    paired[n] = opener;
}

// On removing a closer, retrieve its paired opener, which now needs to be
// pushed on the active stack.
int fetchOpener(int *paired) {
    adjust(paired, -1);
    int n = length(paired);
    return paired[n];
}

// Return the most recent (negative) closer after the cursor.
int topCloser(int *active) {
    int h = high(active);
    int m = max(active);
    if (h == m) return SentinelE;
    return active[h];
}

// Add a new (negative) closer after the cursor. Return the (positive) partner
// opener before the cursor, so the pair can be highlighted.
int pushCloser(int *active, int closer) {
    rehigh(active, -1);
    int h = high(active);
    active[h] = closer;
    int m = max(active);
    int n = length(active);
    if (n >= m - h) return active[m - h - 1];
    else return SentinelB;
}

// Remove the (-ve) top closer after the cursor. Return the (+ve) paired
// opener before the cursor, which now needs to be marked as unmatched.
int popCloser(int *active) {
    rehigh(active, +1);
    int n = length(active);
    int h = high(active);
    int m = max(active);
    if (n >= m - h) return active[m - h -1];
    else return SentinelB;
}

// On pairing two brackets after the cursor, remember the (-ve) closer.
void saveCloser(int *passive, int closer) {
    rehigh(passive, -1);
    int h = high(passive);
    passive[h] = closer;
}

// On removing an opener after the cursor, retrieve its (-ve) paired closer,
// which now needs to be pushed on the active stack.
int fetchCloser(int *passive) {
    int h = high(passive);
    rehigh(passive, +1);
    return passive[h];
}

// ---------- Testing ----------------------------------------------------------
#ifdef bracketsTest

// Each test is a string with . for sentinels, ()[]{} for brackets, and
// optionally | for the cursor. The string is scanned to the end, then the
// cursor is moved back to the | position. The expected output has a letter for
// each bracket. Matched pairs are marked with A, B, ... and mismatched pairs
// or unmatched brackets are marked with a, b, ...
static char *tests[] = {
    ".().",
    ".CC.",

    ".()().",
    ".CCFF.",

    ".()[].",
    ".CCFF.",

    ".[()].",
    ".FDDF.",

    ".(].",
    ".cc.",

    ".).",
    ".b.",

    ".| ().",
    "...XX.",

    ".( | ).",
    ".X...X.",
};
static int ntests = (sizeof(tests) / sizeof(char *)) / 2;

// Check if a pair is matched or mismatched.
static bool match(char open, char close) {
    switch (open) {
    case '(': return close == ')';
    case '[': return close == ']';
    case '{': return close == '}';
    default: return false;
    }
}

static void showBuffer(char *name, int *buffer) {
    printf("%s:", name);
    for (int i = 0; i < max(buffer); i++) {
        if (i < length(buffer)) printf(" %d", buffer[i]);
        if (i == length(buffer)) printf(" | ");
        if (i >= high(buffer)) printf("%d ", buffer[i]);
    }
    printf("\n");
}

// Mark a pair as matched or mismatched.
static void mark(char *in, char *out, int opener, int closer, char id) {
    if (match(in[opener], in[closer])) out[opener] = out[closer] = id;
    else out[opener] = out[closer] = tolower(id);
}

static void scan(char *in, char *out, int *active, int *passive) {
    int n = strlen(in);
    char letter = 'A';
    for (int i = 1; i < n-1; i++) {
        char ch = in[i];
        if (ch == '(' || ch == '[' || ch == '{') {
            int closer = pushOpener(active, i);
            mark(in, out, SentinelB, n + closer, letter++);
        }
        else if (ch == ')' || ch == ']' || ch == '}') {
            int opener = topOpener(active);
            int oldCloser = popOpener(active);
            mark(in, out, SentinelB, n + oldCloser, letter++);
            mark(in, out, opener, i, letter++);
            saveOpener(passive, opener);
        }
    }
    out[0] = out[n-1] = '.';
}

static void moveBack(char *in, char *out, int *active, int *passive) {
    int n = strlen(in);
    char letter = 'Z';
    int cursor = n-1;
    for (int i = 1; i < n-1; i++) if (in[i] == '|') cursor = i;
    for (int i = n-2; i > cursor; i--) {
        char ch = in[i];
    showBuffer("active", active);
    showBuffer("passive", passive);
        if (ch == '(' || ch == '[' || ch == '{') {
            // Undo from front
            int oldCloser = popOpener(active);
            printf("markA %d %d\n", SentinelB, n + oldCloser);
            mark(in, out, SentinelB, n + oldCloser, letter++);
            // Add to back
            int closer = topCloser(active);
            int oldOpener = popCloser(active);
            printf("markB %d %d\n", oldOpener, n + SentinelE);
            mark(in, out, oldOpener, n + SentinelE, letter--);
            printf("markC %d %d\n", i, n + closer);
            mark(in, out, i, n + closer, letter--);
            saveCloser(passive, closer);
        }
        else if (ch == ')' || ch == ']' || ch == '}') {
            // Undo from front
            int opener = fetchOpener(passive);
            int oldCloser = pushOpener(active, opener);
            printf("markD %d %d\n", opener, n + oldCloser);
            mark(in, out, opener, n + oldCloser, letter--);
            // Add to back.
            int oldOpener = pushCloser(active, i - n);
            printf("markE %d %d\n", oldOpener, n + SentinelE);
            mark(in, out, oldOpener, n + SentinelE, letter--);
        }
    }
    out[0] = out[n-1] = '.';
}

static void check(char *in, char *expect) {
    int n = strlen(in);
    int *active = newArray(sizeof(int));
    active = ensure(active, n);
    int *passive = newArray(sizeof(int));
    passive = ensure(passive, n);
    char out[n+1];
    for (int i = 0; i < n; i++) out[i] = '.';
    out[n] = '\0';
    scan(in, out, active, passive);
    moveBack(in, out, active, passive);
    freeArray(active);
    freeArray(passive);
    if (strcmp(out, expect) != 0) {
        printf("Test failed. Input, expected output and actual output are:\n");
        printf("%s\n", in);
        printf("%s\n", expect);
        printf("%s\n", out);
        exit(1);
    }
}

int main() {
    for (int i = 0; i < ntests; i++) check(tests[2*i], tests[2*i+1]);
    printf("Brackets module OK\n");
}

#endif
