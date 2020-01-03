// History, undo, redo. Free and open source. See licence.txt.
// TODO: Elide successive 1-char insertions.
// TODO: short-undo
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// A history structure consists of a flexible array of bytes, with a current
// position in the history during undo/redo sequences.
struct history { int current, length, max; char *bs; };

// Add an opcode to record the position in the history, if not at the end when
// a new non-undo-redo edit is recorded.
enum { Up = End + 1 };

history *newHistory() {
    history *h = malloc(sizeof(history));
    *h = (history) { .current=0, .length=0, .max=1000, .bs=malloc(1000) };
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
static inline void saveString(history *h, int n, char const *s) {
    if (h->length + n > h->max) resize(h, n);
    memcpy(&h->bs[h->length], s, n);
    h->length += n;
    h->current = h->length;
}

// An insert or delete is stored as N OP "..." OP and other operations as N OP.
// An opcode is shifted left one bit, and has 1 added to indicate the last edit
// of a user action. The codes for insert and delete take advantage of the fact
// that text never contains '\0'...'\3', so that a search can be made from
// either end of the string to find the length.
static inline void saveOp(history *h, int op) {
    save(h, op << 1);
}

// Add a signed integer argument to the history, packed in bytes with the top
// bit set. There are opcodes on either side with the top bit unset to delimit
// it. If there are no argument bytes, the argument is zero or not needed.
// (Avoid relying on arithmetic right shift of negative integers.)
static void saveInt(history *h, int n) {
    if (n == 0) return;
    if (n < -134217728 || n >= 134217728) save(h, 0x80 | ((n >> 28) & 0x7F));
    if (n < -1048576 || n >= 1048576) save(h, 0x80 | ((n >> 21) & 0x7F));
    if (n < -8192 || n >= 8192) save(h, 0x80 | ((n >> 14) & 0x7F));
    if (n < -64 || n >= 64) save(h, 0x80 | ((n >> 7) & 0xFF));
    save(h, 0x80 | (n & 0x7F));
}

// Save an opcode and integer argument (in reverse order). If this is after an
// undo, record the old position in the history.
static void saveOpArg(history *h, int op, int by) {
    if (h->current != h->length) {
        int delta = h->length - h->current;
        h->current = h->length;
        saveInt(h, delta);
        saveOp(h, Up);
    }
    saveInt(h,by);
    saveOp(h,op);
    h->current = h->length;
}

// Save an opcode and a string (in reverse order).
static void saveOpString(history *h, int op, int n, char const *s) {
    saveString(h,n,s);
    saveOp(h,op);
    h->current = h->length;
}

void saveInsert(history *h, int by, int n, char const *s) {
    saveOpArg(h, Insert, by);
    saveOpString(h, Insert, n, s);
}

void saveDelete(history *h, int by, int n, char const *s) {
    saveOpArg(h, Delete, by);
    saveOpString(h, Delete, n, s);
}

void saveAddCursor(history *h, int by) { saveOpArg(h, AddCursor, by); }
void saveCutCursor(history *h, int by) { saveOpArg(h, CutCursor, by); }
void saveSetCursor(history *h, int by) { saveOpArg(h, SetCursor, by); }
void saveMoveCursor(history *h, int by) { saveOpArg(h, MoveCursor, by); }
void saveMoveBase(history *h, int by) { saveOpArg(h, MoveBase, by); }
void saveMoveMark(history *h, int by) { saveOpArg(h, MoveMark, by); }
void saveEnd(history *h) { if (h->length > 0) h->bs[h->length-1] |= 1; }

// Pop an opcode off the history.
static int popOp(history *h) {
    if (h->current == 0) return 0;
    char ch = h->bs[--h->current];
    return ch >> 1;
}

// Unpack an int from a range of bytes with the top bit set. Avoid left shift of
// negative numbers.
static int unpack(history *h, int start, int end) {
    if (start == end) return 0;
    char ch = h->bs[start];
    bool neg = (ch & 0x40) != 0;
    unsigned int n = neg ? -1 : 0;
    for (int i = start; i < end; i++) n = (n << 7) | (h->bs[i] & 0x7F);
    return n;
}

// Pop an integer backward off the history (for undo).
static int popInt(history *h) {
    int end = h->current, start;
    for (start = end; start > 0 && (h->bs[start-1] & 0x80) != 0; start--) {}
    h->current = start;
    return unpack(h, start, end);
}

// Read an integer forward off the history (for redo).
static int readInt(history *h) {
    int start = h->current, end;
    for (end = start; end < h->length && (h->bs[end] & 0x80) != 0; end++) {}
    h->current = end;
    return unpack(h, start, end);
}

// Pop a string off the history, returning the number of bytes in *pn.
static char const *popString(history *h, int *pn) {
    int i;
    for (i = h->current; i > 0 && h->bs[i-1] >= 4; i--) { }
    *pn = h->current - i;
    h->current = i;
    return &h->bs[h->current];
}

// Read a string from the history, returning the number of bytes in *pn.
static char const *readString(history *h, int *pn) {
    int i;
    for (i = h->current; i < h->length && h->bs[i] >= 4; i++) { }
    *pn = i - h->current;
    int old = h->current;
    h->current = i;
    return &h->bs[old];
}

// Pop an edit off the history.
static edit popEdit(history *h) {
    edit e = { .last=false, .op=0, .by=0, .n=0, .s=NULL };
    e.op = popOp(h);
    if (e.op == Insert || e.op == Delete) {
        e.s = popString(h, &e.n); popOp(h);
    }
    e.by = popInt(h);
    if (h->length == 0 || (h->bs[h->length-1] >> 1) == Up) {
        popOp(h);
        h->current = h->current - popInt(h);
    }
    if (h->length == 0 || (h->bs[h->length-1] & 1) != 0) e.last = true;
    return e;
}

// Read an edit forwards from the history, for redo.
static edit readEdit(history *h) {
    edit e = { .last=false, .op=End, .by=0, .n=0, .s=NULL };
    if (h->current >= h->length) return e;
    e.by = readInt(h);
    char ch = h->bs[h->current++];
    e.last = ch & 1;
    e.op = ch >> 1;
    if (e.op == Insert || e.op == Delete) {
        e.s = readString(h, &e.n);
        ch = h->bs[h->current++];
        e.last = ch & 1;
    }
    return e;
}

static void invert(edit *e) {
    switch (e->op) {
        case Insert: e->op = Delete; break;
        case Delete: e->op = Insert; break;
        case AddCursor: e->op = CutCursor; break;
        case CutCursor: e->op = AddCursor; break;
        case SetCursor: e->by = - e->by; break;
        case MoveCursor: e->by = - e->by; break;
        case MoveBase: e->by = - e->by; break;
        case MoveMark: e->by = - e->by; break;
    }
}

edit undo(history *h) {
    edit e = { .op = End };
    if (h->current == 0) return e;
    e = popEdit(h);
    invert(&e);
    return e;
}

edit redo(history *h) {
    edit e = { .op = End };
    if (h->current == h->length) return e;
    return readEdit(h);
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

// Check that an edit can be saved and undone.
static bool checkUndo(history *h, int op, int by, int n, char *s) {
    edit e = { .op=op, .by=by, .n=n, .s=s };
    h->current = h->length = 0;
    saveOpArg(h, op, by);
    if (n > 0) saveOpString(h, op, n, s);
    edit e2 = undo(h);
    invert(&e);
    return
        h->current==0 && e2.op==e.op && e2.by==e.by && e2.n==e.n &&
        (e.s == NULL || strncmp(e2.s, e.s, e.n) == 0);
}

// Check that edits can be saved and undone.
static void testUndo(history *h) {
    assert(checkUndo(h, Insert, 42, 3, "abc"));
    assert(checkUndo(h, Insert, 0, 3, "abc"));
    assert(checkUndo(h, Delete, 31, 4, "wxyz"));
    assert(checkUndo(h, AddCursor, 100, 0, NULL));
    assert(checkUndo(h, CutCursor, 100, 0, NULL));
    assert(checkUndo(h, SetCursor, 4, 0, NULL));
    assert(checkUndo(h, MoveCursor, 100, 0, NULL));
}

int main() {
    setbuf(stdout, NULL);
    testOps();
    history *h = newHistory();
    testInts(h);
    testUndo(h);
    freeHistory(h);
    printf("History module OK\n");
    return 0;
}

#endif
