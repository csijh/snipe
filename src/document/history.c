// History, undo, redo. Free and open source. See licence.txt.
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// A history structure consists of a tracked current text position, current
// cursor index, and a flexible array of bytes. As well as the length, which is
// the high water mark, there is a current position in the history, during
// undo/redo sequences.
struct history { int at, cursor, current, length, max; char *bs; };

// An insertion is stored as AT INS "..." INS, a deletion as AT DEL "..." DEL a
// set cursor as AT SET C SET and other operations as AT OP. An opcode is even,
// and has 1 added to indicate the last edit of a user action. The codes for INS
// and DEL take advantage of the fact that text never contains '\0'...'\3', so
// that a search can be made from either end of the string to the other to find
// the length. Operation bytes have the top bit unset. An argument AT has bytes
// with the top bit set, and contains a packed, signed integer. If there are no
// argument bytes, the argument is zero or not needed. An AT UP operation
// records the previous point in the history, when undos are followed by a
// normal edit.
enum opcode {
    INS = 0, DEL = 2, NEXT = 4, BACK = 6, ADD = 8, CUT = 10, MOVE = 12, BASE =
    14, MARK = 16, UP = 18
};

history *newHistory() {
    history *h = malloc(sizeof(history));
    *h = (history) { .position = 0, .cursor = 0, .length = 0, .max = 1000; };
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
static inline void push(history *h, char b) {
    if (h->length + 1 > h->max) resize(h, 1);
    h->bs[h->length++] = b;
}

// Add n bytes to the history.
static inline void pushBytes(history *h, int n, char *s) {
    if (h->length + n > h->max) resize(h, n);
    memcpy(&h->bs[h->length], s, n);
}

// Add an integer to the history, in bytes with the top bit set. (Avoid relying
// on arithmetic right shift of negative integers.)
static void pushInt(history *h, int n) {
    if (n < -134217728 || n >= 134217728) push(0x80 | ((n >> 28) & 0x7F));
    if (n < -1048576 || n >= 1048576) push(0x80 | ((n >> 21) & 0x7F));
    if (n < -8192 || n >= 8192) push(0x80 | ((n >> 14) & 0x7F));
    if (n < -64 || n >= 64) push(0x80 | ((n >> 7) & 0xFF));
    push(0x80 | (n & 0x7F));
}

// Pop an integer off the history.
static int popInt(history *h) {
    int end = h->current, start;
    for (start = end; start > 0 && (h->bs[start-1] & 0x80) != 0; start--) {}
    if (start == end) return 0;
    h->current = start;
    signed char p = &h->bs[start];
    int n = (*p << 1) / 2;
    for (int i = start + 1; i < end; i++) {
        n = (n << 7) | (h->bs[i] & 0x7F);
    }
    return n;
}

// Push an edit onto the history.
void pushEdit(history *h, int op, int at, int n, char *s) {
    int delta = at - h->at;
    if (at != 0) pushInt(h, delta);
    h->at = at;
    switch (op) {
        case Insert: push(h,INS); pushBytes(h,n,s); push(h,INS); break;
        case Delete: push(h,DEL); pushBytes(h,n,s); push(h,DEL); break;
        case SetCursor: push(h,SET); pushInt(h,n); push(h,SET); break;
        case AddCursor: push(h,ADD); break;
        case CutCursor: push(h,CUT); break;
        case MoveCursor: push(h,MOVE); break;
        case MoveBase: push(h,BASE); break;
        case MoveMark: push(h,MARK); break;
        case End: h->bs[h->length-1] |= 1;
    }
}

// ----------



#ifdef historyTest

// Check pushing and popping an op.
static bool checkBytecode(history *h, int op, int n) {
    pushBytecode(h, op, n);
    int op2, n2;
    popBytecode(h, &op2, &n2);
    if (length(h->bs) != 0) return false;
    if (op2 != op) return false;
    if (n2 != n) return false;
    return true;
}

// Test operands of different sizes.
static void testOperands() {
    history *h = newHistory();
    assert(checkBytecode(h, Ins, 0));
    assert(checkBytecode(h, Ins, 27));
    assert(checkBytecode(h, Ins, 28));
    assert(checkBytecode(h, Ins, 255));
    assert(checkBytecode(h, Ins, 256));
    assert(checkBytecode(h, Ins, 65535));
    assert(checkBytecode(h, Ins, 65536));
    assert(checkBytecode(h, Ins, 16777215));
    assert(checkBytecode(h, Ins, 16777216));
    assert(checkBytecode(h, Ins, 2147483647));
    freeHistory(h);
}

// Test pushing and popping deletion text.
static void testText() {
    history *h = newHistory();
    pushBytes(h, 10, "abcdefghij");
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
    pushEdit(h, Ins, 5, 1, "", true);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == Ins && at == 5 && n == 1 && last == true);
    assert(length(h->bs) == 0);
    pushEdit(h, Ins, 20, 3, "", false);
    pushEdit(h, Ins, 3, 20, "", true);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == Ins && at == 3 && n == 20 && last == false);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == Ins && at == 20 && n == 3 && last == true);
    assert(length(h->bs) == 0);
    pushEdit(h, DelR, 3, 1, "x", true);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == DelR && at == 3 && n == 1 && last == true);
    assert(strncmp(s, "x", n) == 0);
    assert(length(h->bs) == 0);
    freeHistory(h);
}

int main() {
    setbuf(stdout, NULL);
    testOperands();
    testText();
    testEdit();
    printf("History module OK\n");
    return 0;
}

#endif
