// Snipe editor. Free and open source, see licence.txt.
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

// Pointer-aligned header, prefixed to arrays.
struct header { int length, high, max, unit; void *align[]; };
typedef struct header header;

// There is no harm in starting sizes at 1 unit: all it means is that the first
// few reallocations may be in-place without relocation, so almost cost-free.
// For large arrays, doubling becomes less appropriate, because there can't be
// a rapid series of small increases coming from user actions. So instead,
// sizes are increased a page at a time.
enum { MAX0 = 2, PAGE = 4096 };

void *newArray(int unit) {
    header *h = malloc(sizeof(header) + MAX0 * unit);
    *h = (header) { .length = 0, .high = MAX0, .max = MAX0, .unit = unit };
    return h + 1;
}

void freeArray(void *a) {
    header *h = (header *) a - 1;
    free(h);
}

int length(void *a) {
    header *h = (header *) a - 1;
    return h->length;
}

int high(void *a) {
    header *h = (header *) a - 1;
    return h->high;
}

int max(void *a) {
    header *h = (header *) a - 1;
    return h->max;
}

void moveGap(void *a, int gap) {
    header *h = (header *) a - 1;
    int n = h->length + (h->max - h->high);
    if (gap < 0) gap = n - gap;
    if (gap > n) gap = n;
    if (gap < h->length) {
        char *p = (char *) a;
        int len = (h->length - gap) * h->unit;
        int from = gap * h->unit;
        int to = (h->high * h->unit - len);
        memmove(&p[to], &p[from], len);
        h->high = h->high - (h->length - gap);
        h->length = gap;
    }
    else if (gap > h->length) {
        char *p = (char *) a;
        int len = (gap - h->length) * h->unit;
        int from = h->high * h->unit;
        int to = h->length * h->unit;
        memmove(&p[to], &p[from], len);
        h->high = h->high + (gap - h->length);
        h->length = gap;
    }
}

void *ensure(void *a, int d) {
    header *h = (header *) a - 1;
    int hilen = (h->max - h->high);
    int needed = (h->length + d + hilen);
    int size = h->max;
    if (size >= needed) return h + 1;
    while (size < needed) size = size * 3 / 2;
    h = realloc(h, sizeof(header) + size * h->unit);
    char *p = (char *) (h + 1);
    int from = h->high * h->unit;
    int to = (size - hilen) * h->unit;
    int len = hilen * h->unit;
    if (hilen > 0) memmove(&p[to], &p[from], len);
    h->high = size - hilen;
    h->max = size;
    return h + 1;
}

void *adjust(void *a, int d) {
    a = ensure(a, d);
    header *h = (header *) a - 1;
    h->length += d;
    if (h->length < 0) h->length = 0;
    return h + 1;
}

void *rehigh(void *a, int d) {
    a = ensure(a, -d);
    header *h = (header *) a - 1;
    h->high += d;
    return h + 1;
}

void error(char *format, ...) {
    va_list args;
    fprintf(stderr, "Error: ");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, ".\n");
    exit(1);
}

// ---------- Testing ----------------------------------------------------------
#ifdef arrayTest

// Check that a char gap buffer matches a string.
bool okC(char *b, char *s) {
    if (strlen(s) != max(b)) return false;
    for (int i = 0; i < max(b); i++) {
        if (length(b) <= i && i < high(b)) { if (s[i] != '-') return false; }
        else if (s[i] != b[i]) return false;
    }
    return true;
}

// Check that an int gap buffer matches a string.
bool okI(int *b, char *s) {
    if (strlen(s) != max(b)) return false;
    for (int i = 0; i < max(b); i++) {
        if (length(b) <= i && i < high(b)) { if (s[i] != '-') return false; }
        else if (s[i] != b[i]) return false;
    }
    return true;
}

// Test gap buffer with char items.
void testBufferC() {
    char *b = newArray(sizeof(char));
    b = ensure(b,10);
    assert(okC(b, "-------------"));
    b = adjust(b, +5);
    strcpy(b, "abcde");
    assert(okC(b, "abcde--------"));
    moveGap(b, 2);
    assert(okC(b, "ab--------cde"));
    adjust(b, -1);
    assert(okC(b, "a---------cde"));
    b = ensure(b, 14);
    assert(okC(b, "a---------------cde"));
    moveGap(b,3);
    assert(okC(b, "acd---------------e"));
    adjust(b, +3);
    strcpy(&b[3], "xyz");
    assert(okC(b, "acdxyz------------e"));
    freeArray(b);
}

// Test gap buffer with int items.
void testBufferI() {
    int *b = newArray(sizeof(int));
    b = ensure(b,10);
    assert(okI(b, "-------------"));
    b = adjust(b, +5);
    b[0] = 'a'; b[1] = 'b'; b[2] = 'c'; b[3] = 'd'; b[4] = 'e';
    assert(okI(b, "abcde--------"));
    moveGap(b, 2);
    assert(okI(b, "ab--------cde"));
    adjust(b, -1);
    assert(okI(b, "a---------cde"));
    b = ensure(b, 14);
    assert(okI(b, "a---------------cde"));
    moveGap(b,3);
    assert(okI(b, "acd---------------e"));
    adjust(b, +3);
    b[3] = 'x'; b[4] = 'y'; b[5] = 'z';
    assert(okI(b, "acdxyz------------e"));
    freeArray(b);
}

// Test all the ways in which trailing spaces, trailing blank lines or missing
// final newlines can occur through an insertion or deletion.
int main() {
    testBufferC();
    testBufferI();
    printf("Array module OK\n");
    return 0;
}

#endif
