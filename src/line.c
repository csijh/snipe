// The Snipe editor is free and open source, see licence.txt.
#include "line.h"
#include <stdio.h>
#include <assert.h>

int startLine(ints *lines, int row) {
    if (row == 0) return 0;
    return get(lines, row - 1);
}

// Pretend there is an extra line containing just a newline.
int endLine(ints *lines, int row) {
    if (row == length(lines)) return get(lines, row - 1) + 1;
    return get(lines, row);
}

int lengthLine(ints *lines, int row) {
    return endLine(lines, row) - startLine(lines, row);
}

// Find the row number for a position by binary search.
int findRow(ints *lines, int p) {
    int start = 0, end = length(lines);
    if (p >= startLine(lines, end)) return end;
    while (end > start + 1) {
        int mid = start + (end - start) / 2;
        if (p < startLine(lines, mid)) end = mid;
        else start = mid;
    }
    return start;
}

#ifdef test_line

int main() {
    setbuf(stdout, NULL);
    ints *lines = newInts();
    add(lines, 3);
    add(lines, 6);
    add(lines, 9);
    assert(findRow(lines, 0) == 0);
    assert(findRow(lines, 2) == 0);
    assert(findRow(lines, 3) == 1);
    assert(findRow(lines, 5) == 1);
    assert(findRow(lines, 6) == 2);
    assert(findRow(lines, 8) == 2);
    assert(findRow(lines, 9) == 3);
    freeInts(lines);
    printf("Line module OK\n");
    return 0;
}

#endif
