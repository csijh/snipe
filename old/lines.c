// The Snipe editor is free and open source, see licence.txt.
#include "lines.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Store an array holding the position in the text just after each newline. The
// array is organised as a gap buffer from 0 to top, with the gap between lo and
// hi. The total number of bytes in the text is tracked in max, and entries
// after the gap are stored relative to max.
struct lines {
    int *ends;
    int lo, hi, top, max;
};

lines *newLines() {
    lines *ls = malloc(sizeof(lines));
    int *ends = malloc(6 * sizeof(int));
    *ls = (lines) { .lo = 0, .hi = 6, .top = 6, .max = 0, .ends = ends };
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
        ls->ends[ls->hi] = ls->max - ls->ends[ls->lo];
    }
    while (ls->hi < ls->top && ls->max - ls->ends[ls->hi] <= at) {
        ls->ends[ls->lo] = ls->max - ls->ends[ls->hi];
        ls->hi++;
        ls->lo++;
    }
}

// Resize.
static void resize(lines *ls) {
    int size = ls->top;
    size = size * 3 / 2;
    ls->ends = realloc(ls->ends, size);
    int hilen = ls->top - ls->hi;
    memmove(&ls->ends[size - hilen], &ls->ends[ls->hi], hilen);
    ls->hi = size - hilen;
    ls->top = size;
}

// Insert extra lines when string s is inserted at a given position.
static void insertLines(lines *ls, int at, int n, char const s[n]) {
    ls->max = ls->max + n;
    moveGap(ls, at);
    for (int i = 0; i < n; i++) if (s[i] == '\n') {
        if (ls->lo >= ls->hi) resize(ls);
        ls->ends[ls->lo++] = at + i + 1;
    }
}

// Delete lines when n bytes are deleted at a given position.
static void deleteLines(lines *ls, int at, int n) {
    ls->max = ls->max - n;
    moveGap(ls, at);
    at = at + n;
    while (ls->hi < ls->top && ls->max - ls->ends[ls->hi] <= at) {
        ls->hi++;
    }
}

void changeLines(lines *ls, op *o) {
    int at = atOp(o), n= lengthOp(o);
    if ((flagsOp(o) & Del) != 0) deleteLines(ls, at, n);
    else insertLines(ls, at, n, textOp(o));
}

int countLines(lines *ls) {
    return ls->top - (ls->hi - ls->lo);
}

int startLine(lines *ls, int row) {
    assert(0 <= row && row < countLines(ls));
    if (row == 0) return 0;
    else if (row <= ls->lo) return ls->ends[row - 1];
    else return ls->max - ls->ends[row + (ls->hi - ls->lo) - 1];
}

int endLine(lines *ls, int row) {
    assert(0 <= row && row < countLines(ls));
    if (row < ls->lo) return ls->ends[row];
    else return ls->max - ls->ends[row + (ls->hi - ls->lo)];
}

int lengthLine(lines *ls, int row) {
    return endLine(ls, row) - startLine(ls, row);
}

// Find the row number for a position by binary search.
int findRow(lines *ls, int at) {
    assert(0 <= at && at <= ls->max);
    int start = 0, end = countLines(ls);
    while (end > start) {
        int mid = start + (end - start) / 2;
        int s = endLine(ls, mid);
        if (at < s) end = mid;
        else start = mid + 1;
    }
    return start;
}

#ifdef linesTest

int main() {
    setbuf(stdout, NULL);
    lines *ls = newLines();
    assert(countLines(ls) == 0);
    assert(findRow(ls, 0) == 0);
    insertLines(ls, 0, 3, "ab\n");
    assert(countLines(ls) == 1);
    assert(findRow(ls, 0) == 0);
    assert(findRow(ls, 2) == 0);
    assert(findRow(ls, 3) == 1);
    insertLines(ls, 3, 4, "cde\n");
    assert(countLines(ls) == 2);
    assert(findRow(ls, 0) == 0);
    assert(findRow(ls, 2) == 0);
    assert(findRow(ls, 3) == 1);
    assert(findRow(ls, 6) == 1);
    assert(findRow(ls, 7) == 2);
    insertLines(ls, 7, 5, "fghi\n");
    assert(countLines(ls) == 3);
    assert(findRow(ls, 0) == 0);
    assert(findRow(ls, 2) == 0);
    assert(findRow(ls, 3) == 1);
    assert(findRow(ls, 6) == 1);
    assert(findRow(ls, 7) == 2);
    assert(findRow(ls, 11) == 2);
    assert(findRow(ls, 12) == 3);
    assert(startLine(ls, 0) == 0);
    assert(endLine(ls, 0) == 3);
    assert(lengthLine(ls, 0) == 3);
    assert(startLine(ls, 1) == 3);
    assert(endLine(ls, 1) == 7);
    assert(lengthLine(ls, 1) == 4);
    assert(startLine(ls, 2) == 7);
    assert(endLine(ls, 2) == 12);
    assert(lengthLine(ls, 2) == 5);
    freeLines(ls);
    printf("Lines module OK\n");
    return 0;
}

#endif
