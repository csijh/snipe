// Snipe text handling. Free and open source, see licence.txt.
#include "store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Gap buffer for items of unit size. Indexes are in units. The gap is from 'lo'
// to 'hi'. The end index is also the capacity.
struct buffer { int unit, lo, hi, end; char data[]; };
typedef struct buffer buffer;

// Create a gap buffer for n items of given unit size.
static buffer *newBuffer(int unit, int n) {
    buffer *b = malloc(sizeof(buffer) + n * unit);
    *b = (buffer) { .unit=unit, .lo=0, .hi=n, .end=n };
    return b;
}

// The maximum number of items without a resize.
static int capacity(buffer *b) {
    return b->end;
}

// The number of items stored in a buffer.
static int items(buffer *b) {
    return b->lo + (b->end - b->hi);
}

// static int gap(buffer *b) {
//     return b->hi - b->lo;
// }

// Reallocate a buffer to ensure a gap size of at least n items. Return the
// reallocated buffer.
static buffer *resize(buffer *b, int n) {
    int hilen = (b->end - b->hi);
    int needed = (b->lo + n + hilen);
    int size = b->end;
    if (size >= needed) return b;
    while (size < needed) size = size * 3 / 2;
    b = realloc(b, sizeof(buffer) + size * b->unit);
    int from = b->hi * b->unit;
    int to = (size - hilen) * b->unit;
    int len = hilen * b->unit;
    if (hilen > 0) memmove(&b->data[to], &b->data[from], len);
    b->hi = size - hilen;
    b->end = size;
    return b;
}

// Move a buffer's gap to the given position.
static void move(buffer *b, int gap) {
    int n = items(b);
    if (gap < 0) gap = n - gap;
    if (gap > n) gap = n;
    if (gap < b->lo) {
        int len = (b->lo - gap) * b->unit;
        int from = gap * b->unit;
        int to = (b->hi * b->unit - len);
        memmove(&b->data[to], &b->data[from], len);
        b->hi = b->hi - (b->lo - gap);
        b->lo = gap;
    }
    else if (gap > b->lo) {
        int len = (gap - b->lo) * b->unit;
        int from = b->hi * b->unit;
        int to = b->lo * b->unit;
        memmove(&b->data[to], &b->data[from], len);
        b->hi = b->hi + (gap - b->lo);
        b->lo = gap;
    }
}

static void *point(buffer *b, int at) {
    int n = items(b);
    if (at < 0) at = n - at;
    return b->data + at * b->unit;
}

// Prepare to insert n items into the left end of the gap. Return a pointer to
// the items to be filled in.
static void *push(buffer *b, int n) {
    void *p = &b->data[b->lo * b->unit];
    b->lo += n;
    return p;
}

// Record an deletion of n items to the left of the gap.
static void pop(buffer *b, int n) {
    b->lo -= n;
}

// -----------------------------------------------------------------------------
// A store has two gap buffers, one for the objects and one for the line
// boundaries. It also holds the unit size of the objects and a current row
// position. Both buffers have gaps at the current row position. The line
// boundaries are byte offsets in the objects buffer. For example, with three
// lines (row 0 is "abc", row 1 is "def", row 2 is "ghi") and with current row
// 2, we might have:
//      objects:  "abcdef...ghi"  (with 3-byte gap)
//      lines:    [0, 3, ...9]
struct store { buffer *objects; buffer *lines; int unit, currentRow; };

store *newStore(int unit) {
    store *s = malloc(sizeof(store));
    s->objects = newBuffer(unit, 24);
    s->lines = newBuffer(sizeof(int), 24);
    return s;
}

void freeStore(store *s) {
    free(s->objects);
    free(s->lines);
    free(s);
}

int rows(store *s) {
    return items(s->lines);
}

// Find the start of the given line.
static int start(store *s, int row) {
    int *p = point(s->lines, row);
    return *p;
}

// Find the end of the given line.
static int end(store *s, int row) {
    if (row == s->currentRow - 1) return s->objects->lo;
    else if (row == rows(s) - 1) return s->objects->end;
    else return start(s, row + 1);
}

int cols(store *s, int row) {
    return end(s, row) - start(s, row);
}

void *fetch(store *s, int row) {
    return &s->objects[start(s, row)];
}

void *insert(store *s, position p, int n) {
    move(s->objects, start(s, p.row) + p.col);
    void *ptr = point(s->objects, n);
    move(s->objects, end(s, p.row));
    // lines gap
    return ptr;
}

// // Move the store gaps to the given row position.
// static void moveGaps(store *s, int row) {
//     if (row < s->currentRow) {
//         move(s->objects, start(s, row));
//         for (int r = row; r < s->currentRow; r++) {
//             s->lines[r] += (s->objects->hi - s->objects->lo);
//         }
//         move(s->lines, row);
//     }
//     else if (row > s->currentRow) {
//         move(s->objects, start(s, row) - (s->objects->hi - s->objects->lo));
//         move(s->lines, row);
//         for (int r = s->currentRow; r < row; r++) {
//             s->lines[r] -= (s->objects->hi - s->objects->lo);
//         }
//     }
// }
//
//     s->objects = resize(s->objects, n * s->unit);
//     moveGaps(p.row + 1);
//     int insertPoint = s->lines[p.row] + p.col * s->unit;
//     int rest = s->lines[p.row + 1] - insertPoint;
//     s->lines[p.row + 1] += n * s->unit;
//     char *to = &s->objects->data[insertPoint + n*s->unit];
//     char *from = &s->objects->data[insertPoint];
//     memmove(to, from, rest);
//     return from;
// }
//
// //-----------------------
//
// void *delete(store *b, position p, int n);
// void split(store *b, position p);
// void join(store *b, int row);
//
//
//
//
//
//
//
// // Return a volatile pointer to the subarray from position p to position q.
// // objects at positions before p can be accessed using negative indexes, i.e.
// // the gap is at position q or beyond.
// void *point(buffer *b, int p, int q);
//
// // Prepare for an insert of n objects at position p. Return a volatile pointer
// // to the subarray of n objects to be filled in. May cause a reallocation.
// void *insert(buffer *b, int p, int n);
//
// // Prepare for a deletion of n objects at position p. Return a volatile pointer
// // to the subarray of n items to be copied out, if desired.
// void *delete(buffer *b, int p, int n);
//
//
//
//
//
// // ----------------------------------------
//
// void insert(buffer *b, int n) {
//     b->lo = b->lo + n;
//     if (b->lo > b->hi) b->lo = b->hi;
// }
//
// // Record a deletion of n bytes from the gap.
// void delete(buffer *b, int n) {
//     b->lo = b->lo - n;
//     if (b->lo < 0) b->lo = 0;
// }

// Unit testing
#ifdef TESTstore

// Check that a buffer of bytes matches a string.
static bool ok1(buffer *b, char *s) {
    int lo = strchr(s, '[') - s;
    int hi = strchr(s, ']') + 1 - s;
    int end = strlen(s);
    if (b->lo != lo || b->hi != hi || b->end != end) return false;
    for (int i = 0; i < b->end; i++) {
        if (i >= b->lo && i < b->hi) continue;
        if (b->data[i] != s[i]) return false;
    }
    return true;
}

// Check that a buffer of ints matches a string.
static bool ok4(buffer *b, char *s) {
    int lo = strchr(s, '[') - s;
    int hi = strchr(s, ']') + 1 - s;
    int end = strlen(s);
    if (b->lo != lo || b->hi != hi || b->end != end) return false;
    for (int i = 0; i < b->end; i++) {
        if (i >= b->lo && i < b->hi) continue;
        int *p = (int *) b->data;
        if (p[i] != s[i]) return false;
    }
    return true;
}

// Test buffer with char items.
static void testBufferC() {
    buffer *b = newBuffer(1, 10);
    assert(ok1(b, "[--------]"));
    char *p = push(b, 5);
    strncpy(p, "abcde", 5);
    assert(ok1(b, "abcde[---]"));
    move(b, 2);
    assert(ok1(b, "ab[---]cde"));
    pop(b, 1);
    assert(ok1(b, "a[----]cde"));
    b = resize(b, 10);
    assert(ok1(b, "a[---------]cde"));
    move(b, 3);
    assert(ok1(b, "acd[---------]e"));
    p = push(b, 3);
    strncpy(p, "xyz", 3);
    assert(ok1(b, "acdxyz[------]e"));
    free(b);
}

// Test buffer with int items.
static void testBufferI() {
    buffer *b = newBuffer(sizeof(int), 10);
    assert(ok4(b, "[--------]"));
    int *p = push(b, 5);
    p[0] = 'a'; p[1] = 'b'; p[2] = 'c'; p[3] = 'd'; p[4] = 'e';
    assert(ok4(b, "abcde[---]"));
    move(b, 2);
    assert(ok4(b, "ab[---]cde"));
    pop(b, 1);
    assert(ok4(b, "a[----]cde"));
    b = resize(b, 10);
    assert(ok4(b, "a[---------]cde"));
    move(b, 3);
    assert(ok4(b, "acd[---------]e"));
    p = push(b, 3);
    p[0] = 'x'; p[1] = 'y'; p[2] = 'z';
    assert(ok4(b, "acdxyz[------]e"));
    free(b);
}

static void testStore0() {
    store *s = newStore(1);
    assert(rows(s) == 0);
    freeStore(s);
}

// Test all the ways in which trailing spaces, trailing blank lines or missing
// final newlines can occur through an insertion or deletion.
int main() {
    testBufferC();
    testBufferI();
    testStore0();
    printf("State module OK\n");
    return 0;
}

#endif
