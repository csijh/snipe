// Snipe editor. Free and open source, see licence.txt.
#include "array.h"
#include "brackets.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// A brackets object has two gap buffers, one for active brackets, and one for
// inactive brackets which have been paired, i.e. matched or mismatched. The
// low part of the active buffer holds openers before the cursor which are
// unpaired up to that point. The high part contains unpaired closers, which
// are the result of backward bracket matching from the end of the text to the
// cursor. The inactive buffer allows bracket matching to be undone, and the
// low part contains open brackets which have been paired during forward
// matching in the order their closers appeared. The high part similarly
// contains paired closers after the cursor. The brackets object also tracks
// the number of outdenters and indenters on the current line, during forward
// matching of the line.
struct brackets {
    int *active, *inactive;
    int outdenters, indenters;
};

Brackets *newBrackets() {
    Brackets *bs = malloc(sizeof(Brackets));
    bs->active = newArray(sizeof(int));
    bs->inactive = newArray(sizeof(int));
    bs->outdenters = 0;
    bs->indenters = 0;
    return bs;
}

void freeBrackets(Brackets *bs) {
    freeArray(bs->active);
    freeArray(bs->inactive);
    free(bs);
}

// Mark a bracket as mismatched or unmatched.
static void markBad(Type *ts, int bracket) {
    setByte(ts, bracket, getByte(ts, bracket) | Bad);
}

// Mark a bracket as matched.
static void markGood(Type *ts, int bracket) {
    setByte(ts, bracket, getByte(ts, bracket) & ~Bad);
}

// Mark two brackets as matched or mismatched.
static void mark(Type *ts, int opener, int closer) {
    if (bracketMatch(getByte(ts, opener), getByte(ts, closer))) {
        markGood(ts, opener);
        markGood(ts, closer);
    }
    else {
        markBad(ts, opener);
        markBad(ts, closer);
    }
}

// Pre-allocate buffers to avoid relocation during scanning. Initialize counts.
void startLine(Brackets *bs, Type *ts, int lo, int hi) {
    int n = hi - lo;
    bs->active = ensure(bs->active, n);
    bs->inactive = ensure(bs->inactive, n);
    bs->outdenters = bs->indenters = 0;
}

int topOpener(Brackets *bs) {
    int n = length(bs->active);
    if (n == 0) return MISSING;
    return bs->active[n-1];
}

// Push opener, and highlight it and the paired backward closer.
void pushOpener(Brackets *bs, Type *ts, int opener) {
    if (opener == MISSING) return;
    int n = length(bs->active);
    adjust(bs->active, +1);
    bs->active[n] = opener;
    int h = high(bs->active);
    int m = max(bs->active);
    int closer = MISSING;
    if (h <= m - n - 1) closer = bs->active[m - n - 1];
    mark(ts, opener, closer);
    bs->indenters++;
}

// Undo pushOpener. Mark the old paired closer after the cursor as unmatched.
static int popOpener(Brackets *bs, Type *ts) {
    int n = length(bs->active);
    if (n == 0) return MISSING;
    int opener = bs->active[n-1];
    adjust(bs->active, -1);
    int h = high(bs->active);
    int m = max(bs->active);
    int oldCloser = MISSING;
    if (h <= m - n - 1) oldCloser = bs->active[m - n - 1];
    markBad(ts, oldCloser);
    return opener;
}

// On pairing an opener with a closer, remember the opener.
static void saveOpener(Brackets *bs, int opener) {
    int n = length(bs->inactive);
    adjust(bs->inactive, +1);
    bs->inactive[n] = opener;
}

void matchCloser(Brackets *bs, Type *ts, int closer) {
    int opener = popOpener(bs, ts);
    saveOpener(bs, opener);
    mark(ts, opener, closer);
    if (bs->indenters == 0) bs->outdenters++;
    else bs->indenters--;
}

int outdenters(Brackets *bs) {
    return bs->outdenters;
}

int indenters(Brackets *bs) {
    return bs->indenters;
}

// Retrieve a previously saved opener.
static int fetchOpener(Brackets *bs) {
    adjust(bs->inactive, -1);
    int n = length(bs->inactive);
    return bs->inactive[n];
}

void matchForward(Brackets *bs, Type *ts, int lo, int hi) {
    for (int i = lo; i < hi; i++) {
        if (isOpener(getByte(ts,i))) pushOpener(bs, ts, i);
        else if (isCloser(getByte(ts,i))) matchCloser(bs, ts, i);
    }
}

// Undo forward matching between lo and hi.
void clearForward(Brackets *bs, Type *ts, int lo, int hi) {
    for (int i = hi-1; i >= lo; i--) {
        if (isOpener(getByte(ts,i))) popOpener(bs, ts);
        else if (isCloser(getByte(ts,i))) pushOpener(bs, ts, fetchOpener(bs));
    }
}

// Add a new closer. Highlight it and the paired opener before the cursor as
// matched or mismatched.
static void pushCloser(Brackets *bs, Type *ts, int closer) {
    int h = high(bs->active);
    rehigh(bs->active, -1);
    bs->active[h-1] = closer;
    int n = length(bs->active);
    int m = max(bs->active);
    int opener = MISSING;
    if (n > m - h - 1) opener = bs->active[m - h - 1];
    mark(ts, opener, closer);
}

// Undo pushCloser. Mark the old paired opener before the cursor as unmatched.
static int popCloser(Brackets *bs, Type *ts) {
    int h = high(bs->active);
    if (h == max(bs->active)) return MISSING;
    int closer = bs->active[h];
    rehigh(bs->active, +1);
    int n = length(bs->active);
    int m = max(bs->active);
    int oldOpener = MISSING;
    if (n > m - h - 1) oldOpener = bs->active[m - h - 1];
    markBad(ts, oldOpener);
    return closer;
}

// On pairing a closer with an opener, remember the closer.
static void saveCloser(Brackets *bs, Type *ts, int closer) {
    rehigh(bs->inactive, -1);
    int h = high(bs->inactive);
    bs->inactive[h] = closer;
}

// Match an opener with the top closer.
static void matchOpener(Brackets *bs, Type *ts, int opener) {
    int closer = popCloser(bs, ts);
    saveCloser(bs, ts, closer);
    mark(ts, opener, closer);
}

// Match backward.
static void matchBackward(Brackets *bs, Type *ts, int hi, int lo) {
    for (int i = lo-1; i >= hi; i--) {
        if (isCloser(getByte(ts,i))) pushCloser(bs, ts, i);
        else if (isOpener(getByte(ts,i))) matchOpener(bs, ts, i);
    }
}

// Retrieve a previously saved closer.
static int fetchCloser(Brackets *bs, Type *ts) {
    int h = high(bs->inactive);
    rehigh(bs->inactive, +1);
    return bs->inactive[h];
}

// Undo the backward matching of brackets between lo and hi.
void clearBackward(Brackets *bs, Type *ts, int lo, int hi) {
    for (int i = lo; i < hi; i++) {
        if (isCloser(getByte(ts,i))) popCloser(bs, ts);
        else if (isOpener(getByte(ts,i))) pushCloser(bs, ts, fetchCloser(bs, ts));
    }
}

//==============================================================================

static void printBuffer(char *name, int *b) {
    printf("%s: ", name);
    for (int i = 0; i < length(b); i++) printf("%d ", b[i]);
    printf("|");
    for (int i = high(b); i < max(b); i++) printf(" %d", b[i]);
    printf("\n");
}

/*
static void printTypes(Type *ts) {
    for (int i = 0; i < length(ts); i++) printf("%c", visualType(getByte(ts,i)));
    for (int i = length(ts); i < high(ts); i++) printf("_");
    for (int i = high(ts); i < max(ts); i++) printf("%c", visualType(getByte(ts,i)));
    printf("\n");
}
*/
/*
void clearLine(Brackets *bs, Type *ts, int p) {
    clearForward(bs, ts, p, bs->cursor);
    clearBackward(bs, ts, bs->cursor, length(ts));
}

void moveBrackets(Brackets *bs, Type *ts, int p) {
    if (p > bs->cursor) {
        moveGap(ts, bs->cursor);
        clearBackward(bs, ts, bs->cursor, p);
        moveGap(ts, p);
        matchForward(bs, ts, bs->cursor, p);
        bs->cursor = p;
    }
    else {
        moveGap(ts, bs->cursor);
        clearForward(bs, ts, p, bs->cursor);
        moveGap(ts, p);
printBuffer("A active", bs->active);
printBuffer("A inactv", bs->inactive);
printTypes(ts);
        int gap = high(ts) - length(ts);
printf("gap %d %d %d\n", gap, gap+p, gap+bs->cursor);
        matchBackward(bs, ts, gap+p, gap+bs->cursor);
        bs->cursor = p;
    }
}
*/
// ---------- Testing ----------------------------------------------------------
#ifdef bracketsTest

// Each test is a string with . for sentinels, ()[]{} for brackets, and
// optionally | for the cursor. The string is scanned to the end, then the
// cursor is moved back to the | position. The expected output has a letter for
// each bracket. Active pairs are marked A, B, ... or a, b, ... if mismatched
// or unmatched. Inactive pairs are marked Z, Y, ... or z, y, ... if mismatched
// or unmatched.
static char *tests[] = {
    ".().",
    ".ZZ.",

    ".()().",
    ".ZZYY.",

    ".()[].",
    ".ZZYY.",

    ".[()].",
    ".YZZY.",

    ".(].",
    ".zz.",

    ".).",
    ".z.",

    ".| ().",
    ".  ZZ.",

    ".( | ).",
    ".X...X.",
};
static int ntests = (sizeof(tests) / sizeof(char *)) / 2;

// Convert an input string into a byte array of types.
static Type *convertIn(char *in) {
    Type *ts = newArray(sizeof(Type));
    int n = strlen(in);
    ts = adjust(ts, n);
    for (int i = 0; i < strlen(in); i++) {
        switch (in[i]) {
        case '(': setByte(ts,i,RoundB); break;
        case '[': setByte(ts,i,SquareB); break;
        case '{': setByte(ts,i,BlockB); break;
        case ')': setByte(ts,i,RoundE); break;
        case ']': setByte(ts,i,SquareE); break;
        case '}': setByte(ts,i,BlockE); break;
        default:  setByte(ts,i,Gap); break;
        }
    }
    return ts;
}

// Convert a brackets object and types array into an output string.
static char *convertOut(Brackets *bs, Type *ts) {
    char *out = newArray(1);
    out = adjust(out, total(ts));
    for (int i = 0; i < total(ts); i++) setChar(out, i, ' ');
    moveGap(out, length(ts));
    for (int i = 0; i < length(bs->active); i++) {
        char letter = 'A' + i;
        if ((getByte(ts,i) & Bad) != 0) letter = 'a' + i;
        setChar(out, bs->active[i], letter);
    }
    for (int i = max(bs->active) - 1; i >= high(bs->active); i--) {
        int m = max(bs->active);
        char letter = 'A' + (m - i - 1);
        if ((getByte(ts,i) & Bad) != 0) letter = 'a' + (m - i - 1);
        setChar(out, bs->active[i], letter);
    }
    int j = 0;
    for (int i = 0; i < length(ts); i++) {
        if (! isCloser(getByte(ts,i))) continue;
        int opener = bs->inactive[j];
        char letter = 'Z' - j;
        if ((getByte(ts,i) & Bad) != 0) letter = 'z' - j;
        setChar(out, opener, letter);
        setChar(out, i, letter);
        j++;
    }
    j = 0;
    for (int i = max(ts) - 1; i >= high(ts); i--) {
        if (! isOpener(getByte(ts,i))) continue;
        int closer = bs->inactive[max(bs->inactive) - 1 - j];
        char letter = 'Z' + j;
        if ((getByte(ts,i) & Bad) != 0) letter = 'z' + j;
        setChar(out, i, letter);
        setChar(out, closer, letter);
        j++;
    }
    return out;
}

static void check(char *in, char *expect) {
    Brackets *bs = newBrackets();
    Type *ts = convertIn(in);
    int n = length(ts);
    startLine(bs, ts, 0, n);
    matchForward(bs, ts, 0, n);
    char *out = convertOut(bs, ts);
    clearForward(bs, ts, 0, n);
    moveGap(ts, 0);
    matchBackward(bs, ts, high(ts)-max(ts), 0);
    printBuffer("a", bs->active);
    printBuffer("b", bs->inactive);
    freeArray(out);
    freeArray(ts);
    freeBrackets(bs);
}

/*
// Do bracket matching on the whole of the input. If there is a cursor, move it
// back to the start, then forward.

    char out[n+1];
    for (int i = 0; i < n; i++) out[i] = ' ';
    out[n] = '\0';
    int cursor = -1;
    for (int i = 0; i < n; i++) if (in[i] == '|') cursor = i;
    if (cursor >= 0) {
        moveGap(ts, n);
printBuffer("a", bs->active);
printBuffer("b", bs->inactive);
        moveBrackets(bs, ts, 1);
printBuffer("c", bs->active);
printBuffer("d", bs->inactive);
        moveGap(ts, 1);
        moveBrackets(bs, ts, cursor);
        moveGap(ts, cursor);
    }
    convertOut(bs, ts, out);
    freeArray(ts);
    freeBrackets(bs);
    if (strcmp(out, expect) == 0) return;
    printf("Test failed. Input, expected output and actual output are:\n");
    printf("%s\n", in);
    printf("%s\n", expect);
    printf("%s\n", out);
    exit(1);
}
*/
int main() {
    for (int i = 0; i < ntests; i++) check(tests[2*i], tests[2*i+1]);
    printf("Brackets module OK\n");
}

#endif
