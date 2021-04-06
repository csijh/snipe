// History, undo, redo. Free and open source. See licence.txt.
// TODO: Elide successive 1-char insertions.
// TODO: short-undo
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// A history structure consists of a flexible array of bytes, plus the text
// position of the most recent insert/delete, plus the current position in the
// history, which is not at the end during undo/redo sequences.
struct history { int position, current, length, max; char *bs; };

// These are even bytes which are not valid in UTF-8, used as opcodes. The odd
// bytes which follow are also invalid, so the bottom bit can be used as a LAST
// flag. INS and DEL refer to plain insertion and deletion of text, GO moves the
// current position in the text, SP indicates the insertion or deletion of a
// number of spaces, UP records the previous current position in the history, if
// not at the end when a new non-undo-redo edit is recorded, and OP records an
// edit operation other than an insert or delete.
enum code {
    INS = 0xC0, DEL = 0xF6, GO = 0xF8, SP = 0xFA, UP = 0xFC, OP = 0xFE, LAST = 1
};

// Check if a byte is valid UFT-8.
static inline bool valid(unsigned char *c) {
    return (c != 0xC0 && c != 0xC1 && c < 0xF5);
}

history *newHistory() {
    history *h = malloc(sizeof(history));
    *h = (history) { .current=0, .length=0, .max=1000, .bs=malloc(1000) };
    return h;
}

void freeHistory(history *h) {
    free(h->bs);
    free(h);
}

void clearHistory(history *h) {
    h->position = h->length = h->current = 0;
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

// Pop a byte from the history.
static inline int pop(history *h, char b) {
    if (h->current == 0) return 0;
    return h->bs[--h->current];
}

// Add a signed integer argument to the history, packed in bytes with the top
// bit unset. If there are no argument bytes, the argument is zero or not
// needed. The bytes are terminated at either end by a byte with the top bit
// set. (Avoid relying on arithmetic right shift of negative integers.)
static void saveInt(history *h, int n) {
    if (n == 0) return;
    if (n < -134217728 || n >= 134217728) save(h, (n >> 28) & 0x7F);
    if (n < -1048576 || n >= 1048576) save(h, (n >> 21) & 0x7F);
    if (n < -8192 || n >= 8192) save(h, (n >> 14) & 0x7F);
    if (n < -64 || n >= 64) save(h, (n >> 7) & 0x7F);
    save(h, n & 0x7F);
}

// Unpack an int from a range of bytes with the top bit set. Avoid left shift of
// negative numbers.
static int unpack(history *h, int start, int end) {
    if (start == end) return 0;
    char ch = h->bs[start];
    bool neg = (ch & 0x40) != 0;
    unsigned int n = neg ? -1 : 0;
    for (int i = start; i < end; i++) n = (n << 7) | (h->bs[i]);
    return n;
}

// Pop an integer backward off the history (for undo).
static int popInt(history *h) {
    int end = h->current, start;
    for (start = end; start > 0 && valid(h->bs[start-1]) == 0; start--) {}
    h->current = start;
    return unpack(h, start, end);
}

// Read an integer forward off the history (for redo).
static int readInt(history *h) {
    int start = h->current, end;
    for (end = start; end < h->length && (h->bs[end] & 0x80) == 0; end++) {}
    h->current = end;
    return unpack(h, start, end);
}

// Add n bytes of text to the history.
static inline void saveText(history *h, int n, char const *s) {
    if (h->length + n > h->max) resize(h, n);
    memcpy(&h->bs[h->length], s, n);
    h->length += n;
    h->current = h->length;
}

// Pop text off the history, returning the number of bytes in *pn.
static char const *popText(history *h, int *pn) {
    int i;
    for (i = h->current; i > 0 && valid(h->bs[i-1]); i--) { }
    *pn = h->current - i;
    h->current = i;
    return &h->bs[h->current];
}

// Read text forward from the history, returning the number of bytes in *pn.
static char const *readText(history *h, int *pn) {
    int i;
    for (i = h->current; i < h->length && valid(h->bs[i]); i++) { }
    *pn = i - h->current;
    int old = h->current;
    h->current = i;
    return &h->bs[old];
}

// Operations are stored as N OP or TEXT OP with a number or text operand and an
// opcode. Numbers are stored with variable length, using bytes with the top bit
// clear, and text is UTF-8 valid, so opcodes serve as both forward and backward
// terminators for the operands. Putting the opcode last makes it easier to
// access the most recent operation. If a new (non-undo-redo) op is saved after
// an undo-redo sequence, record the old position in the history.
static void saveNumberOp(history *h, int op, int n) {
    if (h->current != h->length) {
        int delta = h->length - h->current;
        saveInt(h, delta);
        saveOp(h, UP);
    }
    saveInt(h, n);
    saveOp(h, op);
}

// Save a text argument and opcode.
static void saveTextOp(history *h, int op, int n, char const *s) {
    if (h->current != h->length) {
        int delta = h->length - h->current;
        saveInt(h, delta);
        saveOp(h, UP);
    }
    saveText(h, n, s);
    saveOp(h, op);
}

// Move the text position to p.
static inline void saveGo(history *h, int p) {
    int delta = p - h->position;
    saveNumberOp(h, GO, delta);
    h->position = p;
}

// Save an insertion, possibly including a deletion of spaces.
static void saveInsert(history *h, edit *e) {
    if (h->position != e->to) saveGo(h, e->to);
    if (e->at != e->to) saveNumberOp(h, SP, e->to - e->at);
    h->position = e->at;
    saveTextOp(h, INS, e->n, e->s);
    h->position += n;
}

static void saveDelete(history *h, edit *e) {
    if (h->position != e->to) saveGo(h, e->to);
    saveTextOp(h, DEL, e->n, e->s);
    h->position = e->at;
}

// Save a non-ins/del operation. Both the argument and the operation are stored
// using the OP code.
static void saveOp(history *h, edit *e) {
    saveNumberOp(h, OP, e->n);
    saveNumberOp(h, OP, e->op);
}

void saveEdit(history *h, edit *e) {
    if (e->op < 0) return;
    if (e->op == INSERT) saveInsert(h, e);
    else if (e->op == DELETE) saveDelete(h, e);
    else saveOp(h, e);
}

void saveEnd(history *h) {
    if (h->length == 0 || h->current != h->length) return;
    h->bs[h->length - 1] |= LAST;
}

/*
// Check whether the next undo was the first in its user action.
bool undoIsLast(history *h);

// Check whether the next redo was the last in its user action.
bool redoIsLast(history *h);

// Get the most recent edit. This should be repeated until undoIsLast returns
// true. The caller is responsible for inverting and executing the operation,
// without recording it in the history.
edit undo(history *h, edit *e);

// Get the most recent undone operation, ready for re-execution. This should be
// repeated until redoIsLast returns true. The caller is responsible for
// executing the operation, without recording it in the history.
edit redo(history *h);
*/


/*
//------------------------------------------------------------------------------
void saveInsert(history *h, int by, int n, char const *s) {
    saveArgOp(h, Insert, by);
    saveStringOp(h, Insert, n, s);
}

void saveDelete(history *h, int by, int n, char const *s) {
    saveArgOp(h, Delete, by);
    saveStringOp(h, Delete, n, s);
}

void saveAddCursor(history *h, int by) { saveArgOp(h, AddCursor, by); }
void saveCutCursor(history *h, int by) { saveArgOp(h, CutCursor, by); }
void saveSetCursor(history *h, int by) { saveArgOp(h, SetCursor, by); }
void saveMoveCursor(history *h, int by) { saveArgOp(h, MoveCursor, by); }
void saveMoveBase(history *h, int by) { saveArgOp(h, MoveBase, by); }
void saveMoveMark(history *h, int by) { saveArgOp(h, MoveMark, by); }
void saveEnd(history *h) { if (h->length > 0) h->bs[h->length-1] |= 1; }

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
*/
#ifdef historyTest
// ----------------------------------------------------------------------------

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
/*
// Check that an edit can be saved and undone.
static bool checkUndo(history *h, int op, int by, int n, char *s) {
    edit e = { .op=op, .by=by, .n=n, .s=s };
    h->current = h->length = 0;
    saveArgOp(h, op, by);
    if (n > 0) saveStringOp(h, op, n, s);
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
*/
int main() {
    setbuf(stdout, NULL);
//    testOps();
    history *h = newHistory();
    testInts(h);
//    testUndo(h);
    freeHistory(h);
    printf("History module OK\n");
    return 0;
}

#endif
