// History, undo, redo. Free and open source. See licence.txt.
// TODO: Elide successive 1-char insertions.
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// A history structure consists of a tracked current text position, current
// cursor index, a current position in the history during undo/redo sequences,
// the length (high water mark) and a flexible array of bytes.
struct history { int at, cursor, current, length, max; char *bs; };

// Add a constant to record the position in the history, if not at the end when
// a non-undo-redo edit is recorded.
enum { Up = End + 1 };

history *newHistory() {
    history *h = malloc(sizeof(history));
    *h = (history) { .at=0, .cursor=0, .current=0, .length=0, .max=1000 };
    h->bs = malloc(h->max);
    return h;
}

void freeHistory(history *h) {
    free(h->bs);
    free(h);
}

// Resize the byte array to fit n more bytes.
static void resize(history *h, int n) {
    while (h->length + n > h->max) h->max = h->max * 3 / 2;
    h->bs = realloc(h->bs, h->max);
}

// Add a byte to the history.
static inline void save(history *h, char b) {
    if (h->length + 1 > h->max) resize(h, 1);
    h->bs[h->length++] = b;
    h->current = h->length;
}

// Add n bytes to the history.
static inline void saveN(history *h, int n, char const *s) {
    if (h->length + n > h->max) resize(h, n);
    memcpy(&h->bs[h->length], s, n);
    h->length += n;
    h->current = h->length;
}

// An insertion is stored as N INS "..." INS, a deletion as N DEL "..." DEL and
// other operations as N OP. An opcode is shifted left one bit, and has 1 added
// to indicate the last edit of a user action. The codes for INS and DEL take
// advantage of the fact that text never contains '\0'...'\3', so that a search
// can be made from either end of the string to the other to find the length. An
// argument N has bytes with the top bit set, and contains a packed, signed
// integer. It has opcodes on either side with the top bit unset to delimit it.
// If there are no argument bytes, the argument is zero or not needed.
static inline void saveOp(history *h, int op) {
    save(h, op << 1);
}

// Add an integer to the history, in bytes with the top bit set. (Avoid relying
// on arithmetic right shift of negative integers.)
static void saveInt(history *h, int n) {
    if (n == 0) return;
    if (n < -134217728 || n >= 134217728) save(h, 0x80 | ((n >> 28) & 0x7F));
    if (n < -1048576 || n >= 1048576) save(h, 0x80 | ((n >> 21) & 0x7F));
    if (n < -8192 || n >= 8192) save(h, 0x80 | ((n >> 14) & 0x7F));
    if (n < -64 || n >= 64) save(h, 0x80 | ((n >> 7) & 0xFF));
    save(h, 0x80 | (n & 0x7F));
}

// save a text position onto the history, as a delta to make it self-inverse.
static void saveAt(history *h, int at) {
    int delta = at - h->at;
    saveInt(h, delta);
    h->at = at;
}

// save an edit onto the history.
void saveEdit(history *h, int op, int at, int n, char const *s) {
    if (h->current != h->length) {
        int delta = h->length - h->current;
        saveInt(h, delta);
        saveOp(h, Up);
    }
    switch (op) {
        case Insert: case Delete:
            saveAt(h,at); saveOp(h,op); saveN(h,n,s); saveOp(h,op); break;
        case SetCursor: saveInt(h,n); saveOp(h,op); break;
        case AddCursor: saveAt(h,at); saveOp(h,op); break;
        case CutCursor: saveOp(h,op); break;
        case MoveCursor: saveAt(h,at); saveOp(h,op); break;
        case MoveBase: saveAt(h,at); saveOp(h,op); break;
        case MoveMark: saveAt(h,at); saveOp(h,op); break;
        case End: h->bs[h->length-1] |= 1;
    }
    h->current = h->length;
}

void saveInsert(history *h, int p, int n, char const *s) {
    saveEdit(h, Insert, p, n, s);
}

void saveDelete(history *h, int p, int n, char const *s) {
    saveEdit(h, Delete, p, n, s);
}

void saveSetCursor(history *h, int n) { saveEdit(h, SetCursor, 0, n, NULL); }
void saveAddCursor(history *h, int p) { saveEdit(h, AddCursor, p, 0, NULL); }
void saveCutCursor(history *h) { saveEdit(h, CutCursor, 0, 0, NULL); }
void saveMoveCursor(history *h, int p) { saveEdit(h, MoveCursor, p, 0, NULL); }
void saveMoveBase(history *h, int p) { saveEdit(h, MoveBase, p, 0, NULL); }
void saveMoveMark(history *h, int p) { saveEdit(h, MoveMark, p, 0, NULL); }
void saveEnd(history *h) { saveEdit(h, End, 0, 0, NULL); }

// Pop an opcode off the history.
static int popOp(history *h) {
    if (h->current == 0) return 0;
    char ch = h->bs[--h->current];
    return ch >> 1;
}

// Pop an integer off the history, avoiding left shift of negative number.
static int popInt(history *h) {
    int end = h->current, start;
    for (start = end; start > 0 && (h->bs[start-1] & 0x80) != 0; start--) {}
    if (start == end) return 0;
    h->current = start;
    char ch = h->bs[start];
    bool neg = (ch & 0x40) != 0;
    unsigned int n = neg ? -1 : 0;
    for (int i = start; i < end; i++) {
        n = (n << 7) | (h->bs[i] & 0x7F);
    }
    return n;
}

// Pop a position off the history.
static int popAt(history *h) {
    int at = h->at;
    int delta = popInt(h);
    int old = h->at + delta;
    h->at = old;
    return at;
}

// Pop a string off the history, returning the number of bytes in *pn.
static char const *popN(history *h, int *pn) {
    int i;
    for (i = h->current - 1; i >= 0 && h->bs[i] >= 4; i--) { }
    *pn = h->current - i;
    h->current = i;
    return &h->bs[h->current];
}

static edit popEdit(history *h) {
    edit e = { .op=0, .at=0, .n=0, .last=false, .s=NULL };
    e.op = popOp(h);
    switch (e.op) {
        case Insert: case Delete:
            e.s = popN(h, &e.n); e.at = popAt(h); break;
        case SetCursor: e.n = popInt(h); break;
        case AddCursor: e.at = popAt(h); break;
        case CutCursor: break;
        case MoveCursor: e.at = popAt(h); break;
        case MoveBase: e.at = popAt(h); break;
        case MoveMark: e.at = popAt(h); break;
    }
    if (h->length == 0 || (h->bs[h->length-1] & 1) != 0) e.last = true;
    return e;
}

#ifdef historyTest
// ----------------------------------------------------------------------------

// Check that insert and delete ops can't clash with text bytes.
static void testOps() {
    assert(Insert <= 1 && Delete <= 1);
}

// Check that an integer can be saved and popped.
static bool checkInt(history *h, int n) {
    h->current = h->length = 0;
    saveInt(h, n);
    int m = popInt(h);
    return (h->current == 0 && m == n);
}

// Check that integers around the boundaries can be saved and popped.
static void testInts(history *h) {
    assert(checkInt(h,0));
    assert(checkInt(h,1));
    assert(checkInt(h,63));
    assert(checkInt(h,64));
    assert(checkInt(h,8191));
    assert(checkInt(h,8192));
    assert(checkInt(h,1048575));
    assert(checkInt(h,1048576));
    assert(checkInt(h,134217727));
    assert(checkInt(h,134217728));
    assert(checkInt(h,2147483647));
    assert(checkInt(h,-1));
    assert(checkInt(h,-64));
    assert(checkInt(h,-65));
    assert(checkInt(h,-8192));
    assert(checkInt(h,-8193));
    assert(checkInt(h,-1048576));
    assert(checkInt(h,-1048577));
    assert(checkInt(h,-134217728));
    assert(checkInt(h,-134217729));
    assert(checkInt(h,-2147483648));
}

// Check that saving an edit and then popping it restores the edit.
static bool checkEdit(history *h, int op, int at, int n, char *s) {
    edit e = { .op=op, .at=at, .n=n, .s=s };
    h->current = h->length = 0;
    saveEdit(h, op, at, n, s);
    printf("c=%d\n", h->current);
    for (int i = 0; i < 6; i++) {
        printf("%d\n", h->bs[i]);
    }

    edit e2 = popEdit(h);
    return
        h->current==0 && e2.op==e.op && e2.at==e.at && e2.n==e.n &&
        strncmp(e2.s, e.s, e.n);
}

static void testUndo(history *h) {
    assert(checkEdit(h, Insert, 42, 3, "abc"));
}

/*

// Test saveing and popping deletion text.
static void testText() {
    history *h = newHistory();
    saveN(h, 10, "abcdefghij");
    assert(length(h->bs) == 10);
    char s[100];
    popBytes(h, 10, s);
    assert(length(h->bs) == 0);
    assert(strncmp(s, "abcdefghij", 10) == 0);
    freeHistory(h);
}

static void testEdit() {
    history *h = newHistory();
    int op, at, n;
    bool last;
    char s[100];
    saveEdit(h, Ins, 5, 1, "", true);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == Ins && at == 5 && n == 1 && last == true);
    assert(length(h->bs) == 0);
    saveEdit(h, Ins, 20, 3, "", false);
    saveEdit(h, Ins, 3, 20, "", true);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == Ins && at == 3 && n == 20 && last == false);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == Ins && at == 20 && n == 3 && last == true);
    assert(length(h->bs) == 0);
    saveEdit(h, DelR, 3, 1, "x", true);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == DelR && at == 3 && n == 1 && last == true);
    assert(strncmp(s, "x", n) == 0);
    assert(length(h->bs) == 0);
    freeHistory(h);
}
*/
int main() {
    setbuf(stdout, NULL);
    testOps();
    history *h = newHistory();
    testInts(h);
    testUndo(h);
    freeHistory(h);
//    testEdit();
    printf("History module OK\n");
    return 0;
}

#endif
