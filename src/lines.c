// The Snipe editor is free and open source, see licence.txt.
#include "lines.h"
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// The structure holds the info at the end of one line. The numbers are assumed
// to be zero at the start of the first line.
struct lines { int end, endState, endIndent; };

// The implicit extra structure at the end of the array is used to store the
// number of valid entries.
static int getValid(lines *ls) { return ls[size(ls)].end; }
static void setValid(lines *ls, int r) { ls[size(ls)].end = r; }

lines *newLines() {
    lines *ls = newArray(sizeof(struct lines));
    setValid(ls, 0);
    return ls;
}

void freeLines(lines *ls) {
    freeArray(ls);
}

lines *addLines(lines **pls, int row, int n) {
    lines *ls = increase(pls, row, n);
    if (row < getValid(ls)) setValid(ls, row);
    return ls;
}

void cutLines(lines *ls, int row, int n) {
    decrease(ls, row, n);
    if (row < getValid(ls)) setValid(ls, row);
}

int countLines(lines *ls) {
    return size(ls);
}

int validLines(lines *ls) {
    return getValid(ls);
}

void invalidateLines(lines *ls, int r) {
    if (r < getValid(ls)) setValid(ls, r);
}

int lineStart(lines *ls, int row) {
    if (row == 0) return 0;
    return ls[row - 1].end;
}

// Line n implicitly contains just a newline.
int lineEnd(lines *ls, int row) {
    int n = size(ls);
    if (row == n) return ls[n - 1].end + 1;
    return ls[row].end;
}

int lineLength(lines *ls, int row) {
    return lineEnd(ls, row) - lineStart(ls, row);
}

int lineState(lines *ls, int row) {
    if (row == 0) return 0;
    return ls[row - 1].endState;
}

int lineIndent(lines *ls, int row) {
    if (row == 0) return 0;
    return ls[row - 1].endIndent;
}

void setLineEnd(lines *ls, int row, int p) {
    ls[row].end = p;
}

void setLineEndState(lines *ls, int row, int s) {
    ls[row].endState = s;
    assert(validLines(ls) >= row - 1);
    setValid(ls, row + 1);
}

void setLineEndIndent(lines *ls, int row, int i) {
    ls[row].endIndent = i;
}

#ifdef test_lines

int main() {
    setbuf(stdout, NULL);
    lines *ls = newLines();
    assert(validLines(ls) == 0);
    addLines(&ls, 0, 5);
    assert(validLines(ls) == 0);
    for (int i=0; i<5; i++) setLineEndState(ls, i, i);
    assert(lineState(ls, 4) == 3);
    assert(validLines(ls) == 5);
    addLines(&ls, 5, 200);
    assert(validLines(ls) == 5);
    printf("Lines module OK\n");
}

#endif
