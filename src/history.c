// The Snipe editor is free and open source, see licence.txt.
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// A history structure consists of a tracked position, a flag to say whether a
// multi-cursor group of edits is under way, and a list of bytes.
struct history { int position; bool inGroup; chars *bs; };

// Edits are stored using a custom bytecode. Each byte has a 3-bit opcode and
// non-negative operand. Operand values 0..27 are stored directly in the
// remaining 5 bits of the opcode byte. Values 28..31 mean that the actual
// operand is stored in 1..4 further bytes.

// A single-cursor edit is stored as an insertion or deletion, optionally
// followed by a Left or Right opcode to specify the position at which the edit
// happened relative to the tracked position. A multi-cursor edit is stored as a
// series of single-cursor edits. The first edit in the sequence is accompanied
// by a Left opcode with operand zero, and the last is accompanied by a Right
// opcode with operand zero.

history *newHistory() {
    history *h = malloc(sizeof(history));
    h->position = 0;
    h->inGroup = false;
    h->bs = newChars();
    return h;
}

void freeHistory(history *h) {
    freeList(h->bs);
    free(h);
}

// Add a bytes to the history.
static void add(chars *cs, char c) {
    int n = length(cs);
    resize(cs, n + 1);
    C(cs)[n] = c;
}

// Push n bytes onto the history.
static void pushBytes(history *h, int n, char *s) {
    int len = length(h->bs);
    resize(h->bs, len + n);
    for (int i = 0; i < n; i++) C(h->bs)[len + i] = s[i];
}

// Pop n bytes off the history into the given array.
static void popBytes(history *h, int n, char s[n]) {
    int len = length(h->bs);
    len = len - n;
    for (int i = 0; i < n; i++) s[i] = C(h->bs)[len + i];
    resize(h->bs, len);
}

// Push a bytecode op onto the history, operand first.
static void pushBytecode(history *h, int op, int n) {
    if (n <= 27) { add(h->bs, (op << 5) | n); return; }
    int count = 0;
    while (n > 0) {
        add(h->bs, n & 0xFF);
        n = n >> 8;
        count++;
    }
    add(h->bs, (op << 5) | (27 + count));
}

// Pop a bytecode op off the history into the given variables.
static void popBytecode(history *h, int *pop, int *pn) {
    int len = length(h->bs);
    unsigned char byte = C(h->bs)[--len];
    *pop = byte >> 5;
    *pn = byte & 0x1F;
    if (*pn >= 28) {
        int n = 0;
        for (int i = 0; i < *pn - 27; i++) {
            n = (n << 8) + (C(h->bs)[--len] & 0xFF);
        }
        *pn = n;
    }
    resize(h->bs, len);
}

// Add two more opcodes.
enum { Left = CutLeft + 1, Right = CutLeft + 2 };

// Push an edit onto the history. Start with the insert/delete, then maybe add
// left/right movement, and a begin/end marker.
void pushEdit(history *h, opcode op, int at, int n, char *s, bool last) {
    if (op != Ins) pushBytes(h, n, s);
    pushBytecode(h, op, n);

    if (at > h->position) pushBytecode(h, Right, at - h->position);
    else if (at < h->position) pushBytecode(h, Left, h->position - at);
    if (op == Ins) h->position = at + n;
    else h->position = at;

    if (! h->inGroup && last) return;
    if (! h->inGroup) {
        pushBytecode(h, Left, 0);
        h->inGroup = true;
    }
    if (last) {
        pushBytecode(h, Right, 0);
        h->inGroup = false;
    }
}

void popEdit(history *h, opcode *op, int *at, int *n, char *s, bool *last) {
    *at = h->position;
    *last = true;
    *n = 0;
    *s = '\0';
    if (length(h->bs) == 0) return;
    bool done = false;
    while (! done) {
        popBytecode(h, op, n);
        switch (*op) {
            case Left:
                if (*n == 0) *last = true;
                else h->position += *n;
                break;
            case Right:
                if (*n == 0) *last = false;
                else h->position -= *n;
                break;
            case Ins:
                h->position -= *n;
                *at -= *n;
                done = true;
                break;
            default:
                popBytes(h, *n, s);
                done = true;
                break;
        }
    }
}

#ifdef test_history

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
    pushEdit(h, DelLeft, 3, 1, "x", true);
    popEdit(h, &op, &at, &n, s, &last);
    assert(op == DelLeft && at == 3 && n == 1 && last == true);
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
