// The Snipe editor is free and open source, see licence.txt.
#include "lines.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Store an array holding the position in the text just after each newline. The
// array is organised as a gap buffer from 0 to top, with the gap between lo and
// hi. The total number of bytes of text is tracked in max, and entries after
// the gap are stored relative to max.
struct lines {
    int *ends;
    int lo, hi, top, max;
};

lines *newLines() {
    lines *ls = malloc(sizeof(lines));
    int *ends = malloc(6 * sizeof(int));
    *ls = (lines) { .length = 0, .capacity = 6, .ends = ends };
    return ls;
}

void freeLines(lines *ls) {
    free(ls->ends);
    free(ls);
}

// Move the gap to the given text position.
static void moveGap(lines *ls, int at) {
    while (ls->lo > 0 && ls->ends[ls->lo - 1] > at) {
        ls->lo--;
        ls->hi--;
        ls->ends[ls->hi] = max - ls->ends[ls->lo];
    }
    while (row > t->lo) {
        ls->ends[ls->lo] = max - ls->ends[ls->hi];
        ls->hi++;
        ls->lo++;
    }
}

// Resize.
static void resize(lines *ls) {
    int size = t->top;
    size = size * 3 / 2;
    t->data = realloc(t->data, size);
    int hilen = t->top - t->hi;
    memmove(&t->data[size - hilen], &t->data[t->hi], hilen);
    t->hi = size - hilen;
    t->top = size;
}

// Insert extra lines when string s is inserted at a given position.
static void insertLines(text *t, int at, int n, char const s[n]) {
    ls->max = ls->max + lengthOp(o);
    moveGap();
    for (int i = 0; i < strlen(s); i++) if (s[i] == '\n') {
        int n = p + i + 1;
        expand(lines, index, 1);
        I(lines)[index++] = n;
    }
}

// Delete lines when n bytes are deleted at position p.
static void deleteLines(text *t, int p, int n) {
    ints *lines = t->lines;
    int count = 0, index = 0;
    int len = length(lines);
    while (index < len && I(lines)[index] <= p) index++;
    for (int i = index; i < len && I(lines)[i] <= p+n; i++) count++;
    if (count > 0) delete(lines, index, count);
}

void changeLines(lines *ls, op *o) {
    if (flagsOp(o) & Del != 0) {
        ls->max = ls->max + lengthOp(o);
    for (int i = 0; i < strlen(s); i++) if (s[i] == '\n') {
        int n = p + i + 1;
        expand(lines, index, 1);
        I(lines)[index++] = n;
    }

    } else {

    }
}

int countLines(lines *ls) {
    return ls->top - (ls->hi - ls->lo);
}

int startLine(lines *ls, int row) {
    assert(0 <= row && row < countLines(ls));
    if (row == 0) return 0;
    else if (row <= ls->lo) return ls->ends[row - 1];
    else return max - ls->ends[row + (ls->hi - ls->lo) - 1];
}

int endLine(lines *ls, int row) {
    assert(0 <= row && row < ls->length);
    if (row < ls->lo) return ls->ends[row];
    else return max - ls->ends[row + (ls->hi - ls->lo)];
}

int lengthLine(lines *ls, int row) {
    return endLine(ls, row) - startLine(ls, row);
}

// Find the row number for a position by binary search.
int findRow(lines *ls, int at) {
    assert(0 <= at && at <= ls->ends[ls->length]);
    int start = 0, end = ls->length;
    bool found = false;
    while (end > start) {
        int mid = start + (end - start) / 2;
        int s = startLine(ls, mid);
        if (at < s) end = mid;
        else start = mid + 1;
    }
    return start;
}

#ifdef linesTest

int main() {
    setbuf(stdout, NULL);
    lines *ls = newLines();
    resize(ls, 3);
    I(ls)[0] = 3; I(ls)[1] = 6; I(ls)[2] = 9;
    assert(findRow(ls, 0) == 0);
    assert(findRow(ls, 2) == 0);
    assert(findRow(ls, 3) == 1);
    assert(findRow(ls, 5) == 1);
    assert(findRow(ls, 6) == 2);
    assert(findRow(ls, 8) == 2);
    assert(findRow(ls, 9) == 3);
    freeList(ls);
    printf("Line module OK\n");
    return 0;
}

#endif
