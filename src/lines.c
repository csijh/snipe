// Snipe editor. Free and open source, see licence.txt.
#include "lines.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// The lines are stored in a gap buffer as the index positions in the text just
// after each newline. The indexes after the gap are relative to the end of the
// text, so that they remain stable across insertions and deletions at the gap.
// Text insertions and deletions are monitored, to track the size of the text,
// move the gap, and add or remove newlines. The gap buffer is
// 0..low..high..max, and the text is 0..end.
struct lines { int low, high, max, end; int *data; };

enum { MAX0 = 2, MUL = 3, DIV = 2 };

// Create or free a lines object.
Lines *newLines() {
    Lines *ls = malloc(sizeof(Lines));
    int *data = malloc(MAX0 * sizeof(int));
    *ls = (Lines) { .low=0, .high=MAX0, .max=MAX0, .end=0, .data=data };
    return ls;
}

void freeLines(Lines *ls) {
    free(ls->data);
    free(ls);
}

int sizeL(Lines *ls) {
    return ls->low + ls->max - ls->high;
}

int startL(Lines *ls, int row) {
    if (row == 0) return 0;
    else if (row <= ls->low) return ls->data[row - 1];
    row = row + ls->high - ls->low;
    if (row >= ls->max) row = ls->max - 1;
    return ls->data[row - 1] + ls->end;
}

int endL(Lines *ls, int row) {
    if (row < ls->low) return ls->data[row];
    row = row + ls->high - ls->low;
    if (row >= ls->max) row = ls->max - 1;
    return ls->data[row] + ls->end;
}

int lengthL(Lines *ls, int row) {
    return endL(ls, row) - startL(ls, row);
}

// Move the gap to position p. Change signs of indexes across the gap.
static void moveL(Lines *ls, int p) {
    while (ls->low > 0 && ls->data[ls->low-1] > p) {
        ls->data[--ls->high] = ls->data[--ls->low] - ls->end;
    }
    while (ls->high < ls->max && ls->end + ls->data[ls->high] <= p) {
        ls->data[ls->low++] = ls->data[ls->high] + ls->end;
    }
}

// Make room for extra lines.
static void ensureL(Lines *ls, int extra) {
    int low = ls->low, high = ls->high, max = ls->max;
    int new = max;
    while (new < low + max - high + extra) new = new * MUL / DIV;
    ls->data = realloc(ls->data, new * sizeof(int));
    if (high < max) {
        memmove(ls->data + high + new - max, ls->data + high, max - high);
    }
    ls->high = high + new - max;
    ls->max = new;
}

void insertL(Lines *ls, int p, char *s, int n) {
    ls->end += n;
    moveL(ls, p);
    for (int i = 0; i < n; i++) if (s[i] == '\n') {
        if (ls->low >= ls->high) ensureL(ls, 1);
        ls->data[ls->low++] = p + i + 1;
    }
}

void deleteL(Lines *ls, int p, char *s, int n) {
    ls->end -= n;
    moveL(ls, p);
    p = p + n;
    while (ls->high < ls->max && ls->data[ls->high] + ls->end <= p) {
        ls->high--;
    }
}

// ---------- Testing ----------------------------------------------------------
#ifdef linesTest

int main() {
    setbuf(stdout, NULL);
    Lines *ls = newLines();
    assert(sizeL(ls) == 0);
    insertL(ls, 0, "ab\n", 3);
    assert(sizeL(ls) == 1);
    insertL(ls, 3, "cde\n", 4);
    assert(sizeL(ls) == 2);
    insertL(ls, 7, "fghi\n", 5);
    assert(sizeL(ls) == 3);
    assert(startL(ls, 0) == 0);
    assert(endL(ls, 0) == 3);
    assert(lengthL(ls, 0) == 3);
    assert(startL(ls, 1) == 3);
    assert(endL(ls, 1) == 7);
    assert(lengthL(ls, 1) == 4);
    assert(startL(ls, 2) == 7);
    assert(endL(ls, 2) == 12);
    assert(lengthL(ls, 2) == 5);
    moveL(ls, 0);
    assert(sizeL(ls) == 3);
    assert(startL(ls, 0) == 0);
    assert(endL(ls, 0) == 3);
    assert(lengthL(ls, 0) == 3);
    assert(startL(ls, 1) == 3);
    assert(endL(ls, 1) == 7);
    assert(lengthL(ls, 1) == 4);
    assert(startL(ls, 2) == 7);
    assert(endL(ls, 2) == 12);
    assert(lengthL(ls, 2) == 5);
    freeLines(ls);
    printf("Lines module OK\n");
    return 0;
}

#endif
