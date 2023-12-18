// Snipe editor. Free and open source, see licence.txt.
#include "brackets.h"
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>

// A gap buffer is used to store a stack of openers (left brackets) before the
// text cursor at one end, and a stack of closers after the cursor at the
// other. Each bracket is represented as an integer index into the text. The
// indexes in the upper stack are negative, relative to the end of the text, to
// make them stable across text insertions and deletions.
struct buffer { int low, high, max, end, *data; };
typedef struct buffer Buffer;

// Initial size, and expansion factor.
enum { MAX0 = 2, MUL = 3, DIV = 2 };

// Value returned on popping an empty stack or failing to match a bracket.
enum { MISSING = INT_MIN };

// Make sure a buffer is big enough for extra items.
static void ensureB(Buffer *b, int extra) {
    int low = b->low, high = b->high, max = b->max;
    int new = max;
    while (new < low + max - high + extra) new = new * MUL / DIV;
    b->data = realloc(b->data, new);
    if (high < max) {
        memmove(b->data + high + new - max, b->data + high, max - high);
    }
    b->high = high + new - max;
    b->max = new;
}

// Push and pop, left and right, assuming sufficient space. MISSIMG may be
// pushed onto the inactive stack. Push and pop on the right deal with storage
// relative to the end of the text. The end of text may be different when
// popping than when pushing, which is what makes the closers stable across
// insertions and deletions.

static void pushL(Buffer *b, int opener) {
    b->data[b->low++] = opener;
}

static int popL(Buffer *b) {
    if (b->low == 0) return MISSING;
    return b->data[--b->low];
}

static void pushR(Buffer *b, int closer) {
    if (closer != MISSING) closer = closer - b->end;
    b->data[--b->high] = closer;
}

static int popR(Buffer *b) {
    if (b->high == b->max) return MISSING;
    int closer = b->data[b->high++];
    if (closer != MISSING) closer = closer + b->end;
    return closer;
}

// On the active stack, get the opener matching the top closer, or the closer
// matching the top opener.

static int getOpener(Buffer *b) {
    int i = b->max - b->high - 1;
    if (b->low <= i) return MISSING;
    return b->data[i];
}

static int getCloser(Buffer *b) {
    int i = b->low - 1;
    if (b->high > b->max - i - 1) return MISSING;
    int closer = b->data[b->max - i - 1];
    if (closer != MISSING) closer = closer + b->end;
    return closer;
}

// A brackets object has two gap buffers, one for active brackets, and one for
// inactive brackets. The active brackets are paired with each other, either
// side of the cursor, from the ends of the text towards the cursor. The
// inactive left brackets have been matched with partners to the left of the
// cursor, and are stacked in the order their partners were found during
// forward matching, allowing that matching to be undone. The inactive right
// brackets are the mirror image. The brackets object also tracks the end of
// the text, stored in each buffer, and the number of outdenters and indenters
// on the current line, during forward matching of the line.
struct brackets {
    Buffer active, inactive;
    int end, outdenters, indenters;
};

Brackets *newBrackets() {
    Brackets *bs = malloc(sizeof(Brackets));
    int *dl = malloc(MAX0 * sizeof(int));
    int *dr = malloc(MAX0 * sizeof(int));
    bs->active = (Buffer) { .low=0, .high=MAX0, .max=MAX0, .end=0, .data=dl };
    bs->inactive = (Buffer) { .low=0, .high=MAX0, .max=MAX0, .end=0, .data=dr };
    bs->outdenters = bs-> indenters = 0;
    return bs;
}

void freeBrackets(Brackets *bs) {
    free(bs->active.data);
    free(bs->inactive.data);
    free(bs);
}

// Mark a bracket as matched, or as mismatched/unmatched.
static void markOne(Text *t, int bracket, bool good) {
    if (good) setK(t, bracket, getK(t, bracket) & ~Bad);
    else setK(t, bracket, getK(t, bracket) | Bad);
}

// Mark two brackets as matched or mismatched.
static void markTwo(Text *t, int opener, int closer) {
    bool good = bracketMatch(getK(t, opener), getK(t, closer));
    markOne(t, opener, good);
    markOne(t, closer, good);
}

// Initialize counts. Pre-allocate buffers to avoid relocation during scanning.
void startLine(Brackets *bs, Text *t, int lo, int hi) {
    int n = hi - lo;
    ensureB(&bs->active, n);
    ensureB(&bs->inactive, n);
    bs->active.end += n;
    bs->inactive.end += n;
    bs->outdenters = bs->indenters = 0;
}

// Check if the top opener matches the given kind of close bracket.
int matchTop(Brackets *bs, Text *t, Kind close) {
    if (bs->active.low == 0) return false;
    int opener = bs->active.data[bs->active.low - 1];
    return bracketMatch(getK(t,opener), close);
}

// Push opener, and re-highlight.
void pushOpener(Brackets *bs, Text *t, int opener) {
    pushL(&bs->active, opener);
    int closer = getCloser(&bs->active);
    if (closer == MISSING) markOne(t, opener, false);
    else markTwo(t, opener, closer);
    bs->indenters++;
}

// Undo pushOpener, and re-highlight.
static int popOpener(Brackets *bs, Text *t) {
    int closer = getCloser(&bs->active);
    int opener = popL(&bs->active);
    if (opener == MISSING) return MISSING;
    if (closer != MISSING) markOne(t, closer, false);
    return opener;
}

// Match a closer with the top opener, and remember the opener, even if MISSING.
void matchCloser(Brackets *bs, Text *t, int closer) {
    int opener = popOpener(bs, t);
    pushL(&bs->inactive, opener);
    if (opener == MISSING) markOne(t, closer, false);
    else markTwo(t, opener, closer);
    if (bs->indenters == 0) bs->outdenters++;
    else bs->indenters--;
}

int outdenters(Brackets *bs) {
    return bs->outdenters;
}

int indenters(Brackets *bs) {
    return bs->indenters;
}

void matchForward(Brackets *bs, Text *t, int lo, int hi) {
    for (int i = lo; i < hi; i++) {
        if (isOpener(getK(t,i))) pushOpener(bs, t, i);
        else if (isCloser(getK(t,i))) matchCloser(bs, t, i);
    }
}

// Undo forward matching between lo and hi.
void clearForward(Brackets *bs, Text *t, int lo, int hi) {
    for (int i = hi-1; i >= lo; i--) {
        if (isOpener(getK(t,i))) popOpener(bs, t);
        else if (isCloser(getK(t,i))) {
            int opener = popL(&bs->inactive);
            if (opener != MISSING) pushOpener(bs, t, opener);
        }
    }
}

// Add a new closer, and re-highlight.
static void pushCloser(Brackets *bs, Text *t, int closer) {
    pushR(&bs->active, closer);
    int opener = getOpener(&bs->active);
    if (opener == MISSING) markOne(t, closer, false);
    else markTwo(t, opener, closer);
}

// Undo pushCloser, and re-highlight.
static int popCloser(Brackets *bs, Text *t) {
    int opener = getOpener(&bs->active);
    int closer = popR(&bs->active);
    if (closer == MISSING) return MISSING;
    if (opener != MISSING) markOne(t, opener, false);
    return closer;
}

// Match an opener with the top closer.
static void matchOpener(Brackets *bs, Text *t, int opener) {
    int closer = popCloser(bs, t);
    pushR(&bs->inactive, closer);
    markTwo(t, opener, closer);
}

// Match backward.
static void matchBackward(Brackets *bs, Text *t, int hi, int lo) {
    for (int i = lo-1; i >= hi; i--) {
        if (isCloser(getK(t,i))) pushCloser(bs, t, i);
        else if (isOpener(getK(t,i))) matchOpener(bs, t, i);
    }
}

// Undo the backward matching of brackets between lo and hi.
void clearBackward(Brackets *bs, Text *t, int lo, int hi) {
    for (int i = lo; i < hi; i++) {
        if (isCloser(getK(t,i))) popCloser(bs, t);
        else if (isOpener(getK(t,i))) {
            int closer = popR(&bs->inactive);
            if (closer != MISSING) pushCloser(bs, t, closer);
        }
    }
}

//==============================================================================
/*
static void printBuffer(char *name, int *b) {
    printf("%s: ", name);
    for (int i = 0; i < length(b); i++) printf("%d ", b[i]);
    printf("|");
    for (int i = high(b); i < max(b); i++) printf(" %d", b[i]);
    printf("\n");
}

static void printTypes(Text *t) {
    for (int i = 0; i < length(t); i++) printf("%c", visualType(getK(t,i)));
    for (int i = length(t); i < high(t); i++) printf("_");
    for (int i = high(t); i < max(t); i++) printf("%c", visualType(getK(t,i)));
    printf("\n");
}

void clearLine(Brackets *bs, Text *t, int p) {
    clearForward(bs, t, p, bs->cursor);
    clearBackward(bs, t, bs->cursor, length(t));
}

void moveBrackets(Brackets *bs, Text *t, int p) {
    if (p > bs->cursor) {
        moveGap(t, bs->cursor);
        clearBackward(bs, t, bs->cursor, p);
        moveGap(t, p);
        matchForward(bs, t, bs->cursor, p);
        bs->cursor = p;
    }
    else {
        moveGap(t, bs->cursor);
        clearForward(bs, t, p, bs->cursor);
        moveGap(t, p);
printBuffer("A active", bs->active);
printBuffer("A inactv", bs->inactive);
printTypes(t);
        int gap = high(t) - length(t);
printf("gap %d %d %d\n", gap, gap+p, gap+bs->cursor);
        matchBackward(bs, t, gap+p, gap+bs->cursor);
        bs->cursor = p;
    }
}
*/
// ---------- Testing ----------------------------------------------------------
#ifdef bracketsTest

// Convert an input string into a text object.
static Text *convertIn(char *in) {
    Text *t = newText();
    int n = strlen(in);
    insertT(t, 0, in, n);
    for (int i = 0; i < n; i++) {
        switch (in[i]) {
        case '(': setK(t,i,RoundB); break;
        case '[': setK(t,i,SquareB); break;
        case '{': setK(t,i,BlockB); break;
        case ')': setK(t,i,RoundE); break;
        case ']': setK(t,i,SquareE); break;
        case '}': setK(t,i,BlockE); break;
        default:  setK(t,i,Gap); break;
        }
    }
    return t;
}

// Convert a brackets object and text into an output string.
static char *convertOut(Brackets *bs, Text *t) {
    char *out = newArray(1);
    out = adjust(out, lengthT(t));
    for (int i = 0; i < lengthT(t); i++) out[i] = ' ';
    for (int i = 0; i < bs->active.low; i++) {
        char letter = 'A' + i;
        if ((getK(t,i) & Bad) != 0) letter = 'a' + i;
        out[bs->active.data[i]] = letter;
    }
    for (int i = bs->active.max - 1; i >= bs->active.high; i--) {
        int m = bs->active.max;
        char letter = 'A' + (m - i - 1);
        if ((getK(t,i) & Bad) != 0) letter = 'a' + (m - i - 1);
        out[bs->active.data[i]] = letter;
    }
    int j = 0;
    for (int i = 0; i < cursorT(t); i++) {
        if (! isCloser(getK(t,i))) continue;
        int opener = bs->inactive.data[j];
        char letter = 'Z' - j;
        if ((getK(t,i) & Bad) != 0) letter = 'z' - j;
        out[opener] = letter;
        out[i] = letter;
        j++;
    }
    j = 0;
    for (int i = lemgthT(t) - 1; i >= cursorT(t); i--) {
        if (! isOpener(getK(t,i))) continue;
        int closer = bs->inactive[max(bs->inactive) - 1 - j];
        char letter = 'Z' + j;
        if ((getK(t,i) & Bad) != 0) letter = 'z' + j;
        out[i] = letter;
        out[closer] = letter;
        j++;
    }
    return out;
}


/*
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

static void check(char *in, char *expect) {
    Brackets *bs = newBrackets();
    Text *t = convertIn(in);
    int n = length(t);
    startLine(bs, t, 0, n);
    matchForward(bs, t, 0, n);
    char *out = convertOut(bs, t);
    clearForward(bs, t, 0, n);
    moveGap(t, 0);
    matchBackward(bs, t, high(t)-max(t), 0);
    printBuffer("a", bs->active);
    printBuffer("b", bs->inactive);
    freeArray(out);
    freeArray(t);
    freeBrackets(bs);
}

// Do bracket matching on the whole of the input. If there is a cursor, move it
// back to the start, then forward.

    char out[n+1];
    for (int i = 0; i < n; i++) out[i] = ' ';
    out[n] = '\0';
    int cursor = -1;
    for (int i = 0; i < n; i++) if (in[i] == '|') cursor = i;
    if (cursor >= 0) {
        moveGap(t, n);
printBuffer("a", bs->active);
printBuffer("b", bs->inactive);
        moveBrackets(bs, t, 1);
printBuffer("c", bs->active);
printBuffer("d", bs->inactive);
        moveGap(t, 1);
        moveBrackets(bs, t, cursor);
        moveGap(t, cursor);
    }
    convertOut(bs, t, out);
    freeArray(t);
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
    Text *t = convertIn("()[]");
    printf("%s\n", kindName(getK(t,3)));
    freeText(t);
//    for (int i = 0; i < ntests; i++) check(tests[2*i], tests[2*i+1]);
    printf("Brackets module OK\n");
}

#endif
