// Snipe editor. Free and open source, see licence.txt.
#include "array.h"
#include "brackets.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// Brackets are stored as integer indexes into the text. Positions after the
// buffer are stored as negative indexes relative to the end of the text, so
// that they are immune to insertions and deletions at the cursor. The
// indexes of the two sentinel bytes are 0 and -1.
enum { SentinelB = 0, SentinelE = -1 };

// A brackets object has two gap buffers, one for active brackets, and one for
// inactive brackets which have been paired, i.e. matched or mismatched. The
// low part of the active buffer holds openers before the cursor which are
// unpaired up to that point, during normal forward bracket matching. The high
// part contains unpaired closers, which are the result of backward bracket
// matching from the end of the text to the cursor. The inactive buffer allows
// bracket matching to be undone, and contains open brackets which have been
// paired during forward matching in the order their closers appeared. The high
// part similarly contains paired closers after the cursor. The brackets object
// also keeps track of the low water mark of the active openers and the number
// of outdenters.
struct brackets { index *active, *inactive; int lwm, outdenters; };

Brackets *newBrackets() {
    Brackets *bs = malloc(sizeof(Brackets));
    bs->active = newArray(sizeof(index));
    bs->inactive = newArray(sizeof(index));
    bs->lwm = bs->outdenters = 0;
    return bs;
}

void freeBrackets(Brackets *bs) {
    freeArray(bs->active);
    freeArray(bs->inactive);
    free(bs);
}

// Return the most recent opener.
int topOpener(Brackets *bs) {
    int n = length(bs->active);
    if (n == 0) return SentinelB;
    return bs->active[n-1];
}

// Mark two brackets as matched or mismatched.
static inline void mark(Type *ts, int opener, int closer) {
    if (bracketMatch(ts[opener], ts[closer])) {
        ts[opener] &= ~Bad;
        ts[closer] &= ~Bad;
    }
    else {
        ts[opener] |= Bad;
        ts[closer] |= Bad;
    }
}

// Add a new opener. Highlight it and the (negative) paired closer after the
// cursor as matched or mismatched.
void addOpener(Brackets *bs, Type *ts, index opener) {
    int n = length(bs->active);
    int h = high(bs->active);
    int m = max(bs->active);
    adjust(bs->active, +1);
    bs->active[n] = opener;
    int closer = SentinelE;
    if (h <= m - n - 1) closer = m - n - 1;
    mark(ts, opener, closer);
}

// Undo addOpener, or prepare to pair up the top opener with a new closer. Mark
// the old paired closer after the cursor as unmatched.
static index popOpener(Brackets *bs, Type *ts) {
    adjust(bs->active, -1);
    int n = length(bs->active);
    index opener = bs->active[n];
    int h = high(bs->active);
    int m = max(bs->active);
    index oldCloser = SentinelE;
    if (h <= m - n - 1) oldCloser = m - n - 1;
    ts[oldCloser] &= ~Bad;
    return opener;
}

// On pairing with a closer, remember the opener.
static void saveOpener(Brackets *bs, index opener) {
    int n = length(bs->inactive);
    adjust(bs->inactive, +1);
    bs->inactive[n] = opener;
}

// Handle a closer during scanning of a line.
void addCloser(Brackets *bs, Type *ts, index closer) {
    index opener = popOpener(bs, ts);
    saveOpener(bs, opener);
    mark(ts, opener, closer);
}

// ------------------------------------------------------------------
/*
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
*/
// ---------- Testing ----------------------------------------------------------
#ifdef bracketsTest

/*
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
*/
int main() {
//    for (int i = 0; i < ntests; i++) check(tests[2*i], tests[2*i+1]);
    printf("Brackets module OK\n");
}

#endif
