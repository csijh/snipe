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
// unpaired up to that point. The high part contains unpaired closers, which
// are the result of backward bracket matching from the end of the text to the
// cursor. The inactive buffer allows bracket matching to be undone, and the
// low part contains open brackets which have been paired during forward
// matching in the order their closers appeared. The high part similarly
// contains paired closers after the cursor. The brackets object also tracks
// the cursor and the number of outdenters and indenters on the current line,
// during forward matching of the line.
struct brackets {
    int *active, *inactive;
    int cursor, outdenters, indenters;
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
    ts[bracket] |= Bad;
}

// Mark a bracket as matched.
static void markGood(Type *ts, int bracket) {
    ts[bracket] &= ~Bad;
}

// Mark two brackets as matched or mismatched.
static void mark(Type *ts, int opener, int closer) {
    if (bracketMatch(ts[opener], ts[closer])) {
        markGood(ts, opener);
        markGood(ts, closer);
    }
    else {
        markBad(ts, opener);
        markBad(ts, closer);
    }
}

// ---------- Forward matching -------------------------------------------------

void startLine(Brackets *bs, Type *ts, int p) {
    int n = length(ts);
    bs->active = ensure(bs->active, n);
    bs->inactive = ensure(bs->inactive, n);
    bs->cursor = n;
    bs->outdenters = bs->indenters = 0;
}

// Return the most recent opener.
int topOpener(Brackets *bs) {
    int n = length(bs->active);
    if (n == 0) return SentinelB;
    return bs->active[n-1];
}

// Add a new opener. Highlight it and the (negative) paired closer after the
// cursor as matched or mismatched.
void pushOpener(Brackets *bs, Type *ts, int opener) {
    int n = length(bs->active);
    int h = high(bs->active);
    int m = max(bs->active);
    adjust(bs->active, +1);
    bs->active[n] = opener;
    int closer = SentinelE + max(ts);
    if (h <= m - n - 1) closer = bs->active[m - n - 1] + max(ts);
    mark(ts, opener, closer);
    bs->indenters++;
}

// Undo pushOpener, either to reverse a matching step, or to prepare to pair up
// the top opener with a new closer. Mark the old paired closer after the
// cursor as unmatched.
static int popOpener(Brackets *bs, Type *ts) {
    int n = length(bs->active);
    if (n == 0) return SentinelB;
    int opener = bs->active[n-1];
    adjust(bs->active, -1);
    int h = high(bs->active);
    int m = max(bs->active);
    int oldCloser = SentinelE + max(ts);
    if (h <= m - n - 1) oldCloser = bs->active[m - n - 1] + max(ts);
    markBad(ts, oldCloser);
    return opener;
}

// On pairing an opener with a closer, remember the opener.
static void saveOpener(Brackets *bs, Type *ts, int opener) {
    int n = length(bs->inactive);
    adjust(bs->inactive, +1);
    bs->inactive[n] = opener;
}

// Match a closer with the top opener.
void matchCloser(Brackets *bs, Type *ts, int closer) {
    int opener = popOpener(bs, ts);
    saveOpener(bs, ts, opener);
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
static int fetchOpener(Brackets *bs, Type *ts) {
    adjust(bs->inactive, -1);
    int n = length(bs->inactive);
    return bs->inactive[n];
}

// Match forward when not scanning.
static void matchForward(Brackets *bs, Type *ts, int from, int to) {
    for (int i = from; i < to; i++) {
        if (isOpener(ts[i])) pushOpener(bs, ts, i);
        else if (isCloser(ts[i])) matchCloser(bs, ts, i);
    }
}

// Undo forward matching of a closer.
static void unmatchCloser(Brackets *bs, Type *ts) {
    int opener = fetchOpener(bs, ts);
    pushOpener(bs, ts, opener);
}

// Undo the forward matching of brackets, backwards from 'from' to 'to'.
static void unmatchForward(Brackets *bs, Type *ts, int to, int from) {
    for (int i = from-1; i >= to; i--) {
        if (isOpener(ts[i])) popOpener(bs, ts);
        else if (isCloser(ts[i])) unmatchCloser(bs, ts);
    }
}

// ---------- Backward matching ------------------------------------------------

// Add a new (negative) closer. Highlight it and the (positive) paired opener
// before the cursor as matched or mismatched.
static void pushCloser(Brackets *bs, Type *ts, int closer) {
    int n = length(bs->active);
    int h = high(bs->active);
    int m = max(bs->active);
    rehigh(bs->active, -1);
    printf("pC %d\n", closer-max(ts));
    bs->active[h-1] = closer - max(ts);
    int opener = SentinelB;
    if (n > m - h - 1) opener = bs->active[m - h - 1];
    mark(ts, opener, closer);
}

// Undo addCloser, either to reverse backward matching, or to prepare to pair up
// the top closer with a new opener. Mark the old paired opener before the
// cursor as unmatched.
static int popCloser(Brackets *bs, Type *ts) {
    int h = high(bs->active);
    if (h == max(bs->active)) return SentinelE + max(ts);
    int closer = bs->active[h] + max(ts);
    rehigh(bs->active, +1);
    int n = length(bs->active);
    int m = max(bs->active);
    int oldOpener = SentinelB;
    if (n > m - h - 1) oldOpener = bs->active[m - h - 1];
    markBad(ts, oldOpener);
    return closer;
}

// On pairing a closer with an opener, remember the closer.
static void saveCloser(Brackets *bs, Type *ts, int closer) {
    rehigh(bs->inactive, -1);
    int h = high(bs->inactive);
    printf("saveCloser %d\n", closer - max(ts));
    bs->inactive[h] = closer - max(ts);
}

// Match an opener with the top closer.
static void matchOpener(Brackets *bs, Type *ts, int opener) {
    int closer = popCloser(bs, ts);
    saveCloser(bs, ts, closer);
    mark(ts, opener, closer);
}

// Retrieve a previously saved closer.
static int fetchCloser(Brackets *bs, Type *ts) {
    int h = high(bs->inactive);
    rehigh(bs->inactive, +1);
    printf("fetchCloser %d\n", bs->inactive[h]);
    return bs->inactive[h] + max(ts);
}

// Undo backward matching of an opener.
static void unmatchOpener(Brackets *bs, Type *ts) {
    int closer = fetchCloser(bs, ts);
    printf("unmatchOpener %d", closer-max(ts));
    pushCloser(bs, ts, closer);
}

static void printBuffer(char *name, int *b) {
    printf("%s: ", name);
    for (int i = 0; i < length(b); i++) printf("%d ", b[i]);
    printf("|");
    for (int i = high(b); i < max(b); i++) printf(" %d", b[i]);
    printf("\n");
}

// Undo the backward matching of brackets, forwards from 'from' to 'to'.
static void unmatchBackward(Brackets *bs, Type *ts, int from, int to) {
    printBuffer("active", bs->active);
    printBuffer("inactv", bs->inactive);
    for (int i = from; i < to; i++) {
        if (isCloser(ts[i])) popCloser(bs, ts);
        else if (isOpener(ts[i])) unmatchOpener(bs, ts);
    }
}

// Match backward.
static void matchBackward(Brackets *bs, Type *ts, int to, int from) {
    for (int i = from-1; i >= to; i--) {
        if (isCloser(ts[i])) {
    printf("matchBackward i=%d %d\n", i, i-max(ts));
            pushCloser(bs, ts, i);
        }
        else if (isOpener(ts[i])) matchOpener(bs, ts, i);
    }
}

void clearLine(Brackets *bs, Type *ts, int p) {
    unmatchForward(bs, ts, p, bs->cursor);
    unmatchBackward(bs, ts, bs->cursor, length(ts));
}

void moveBrackets(Brackets *bs, Type *ts, int p) {
    if (p > bs->cursor) {
        moveGap(ts, bs->cursor);
        unmatchBackward(bs, ts, bs->cursor, p);
        moveGap(ts, p);
        matchForward(bs, ts, bs->cursor, p);
        bs->cursor = p;
    }
    else {
        moveGap(ts, bs->cursor);
        unmatchForward(bs, ts, p, bs->cursor);
        moveGap(ts, p);
printBuffer("A active", bs->active);
printBuffer("A inactv", bs->inactive);
        matchBackward(bs, ts, p, bs->cursor);
        bs->cursor = p;
    }
}

// ---------- Testing ----------------------------------------------------------
#ifdef bracketsTest

// Each test is a string with . for sentinels, ()[]{} for brackets, and
// optionally | for the cursor. The string is scanned to the end, then the
// cursor is moved back to the | position. The expected output has a letter for
// each bracket. Active pairs are marked A, B, ... or a, b, ... if mismatched
// or unmatched. Inactive pairs are marked Z, Y, ... or z, y, ... if mismatched
// or unmatched.
static char *tests[] = {
    /*
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
*/
    ".( | ).",
    ".X...X.",
};
static int ntests = (sizeof(tests) / sizeof(char *)) / 2;

// Convert an input string into a byte array of types.
static void convertIn(char *in, Type *ts) {
    for (int i = 0; i < strlen(in); i++) {
        switch (in[i]) {
        case '(': ts[i] = RoundB; break;
        case '[': ts[i] = SquareB; break;
        case '{': ts[i] = BlockB; break;
        case ')': ts[i] = RoundE; break;
        case ']': ts[i] = SquareE; break;
        case '}': ts[i] = BlockE; break;
        default:  ts[i] = Gap; break;
        }
    }
}

// Convert a brackets object and types array into an output string.
static void convertOut(Brackets *bs, Type *ts, char *out) {
    for (int i = 0; i < length(bs->active); i++) {
        char letter = 'A' + i;
        if ((ts[i] & Bad) != 0) letter = 'a' + i;
        out[bs->active[i]] = letter;
    }
    int j = 0;
    for (int i = 0; i < bs->cursor; i++) {
        if (! isCloser(ts[i])) continue;
        int opener = bs->inactive[j];
        char letter = 'Z' - j;
        if ((ts[i] & Bad) != 0) letter = 'z' - j;
        out[opener] = out[i] = letter;
        j++;
    }
    for (int i = max(bs->active) - 1; i >= high(bs->active); i--) {
        int m = max(bs->active);
        char letter = 'A' + (m - i - 1);
        if ((ts[i] & Bad) != 0) letter = 'a' + (m - i - 1);
        out[bs->active[i] + m] = letter;
    }
    j = 0;
    for (int i = max(ts) - 1; i >= bs->cursor; i--) {
        if (! isOpener(ts[i])) continue;
        int closer = bs->inactive[max(bs->inactive) - 1 - j] + max(ts);
        char letter = 'Z' + j;
        if ((ts[i] & Bad) != 0) letter = 'z' + j;
        out[i] = out[closer] = letter;
        j++;
    }
    out[0] = out[strlen(out)-1] = '.';
}

// Do bracket matching on the whole of the input. If there is a cursor, move it
// back to the start, then forward.
static void check(char *in, char *expect) {
    Brackets *bs = newBrackets();
    Type *ts = newArray(sizeof(Type));
    int n = strlen(in);
    ts = adjust(ts, n);
    convertIn(in, ts);
    startLine(bs, ts, 0);
    matchForward(bs, ts, 0, n);
    char out[n+1];
    for (int i = 0; i < n; i++) out[i] = ' ';
    out[n] = '\0';
    int cursor = -1;
    for (int i = 0; i < n; i++) if (in[i] == '|') cursor = i;
    if (cursor >= 0) {
        moveGap(ts, n);
        moveBrackets(bs, ts, 0);
        moveGap(ts, 0);
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

int main() {
    for (int i = 0; i < ntests; i++) check(tests[2*i], tests[2*i+1]);
    printf("Brackets module OK\n");
}

#endif
