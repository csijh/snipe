// The Snipe editor is free and open source. See licence.txt.
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h>
#include <assert.h>

// Aligned header, prefixed to arrays.
struct header { int unit, length, high, max; void *max_align_t[]; };
typedef struct header header;

// Start small and multiply size by 1.5:
enum { MAX0 = 2, MUL = 3, DIV = 2 };

void *newArray(int unit) {
    header *h = malloc(sizeof(header) + MAX0 * unit);
    *h = (header) { .unit = unit, .length = 0, .high = MAX0, .max = MAX0 };
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

static void setLength(void *a, int n) {
    header *h = (header *) a - 1;
    h->length = n;
}

void *ensure(void *a, int m) {
    header *h = (header *) a - 1;
    int n = h->length + m + h->max - h->high;
    if (h->max >= n) return a;
    int high0 = h->high, max0 = h->max, unit = h->unit;
    while (h->max < n) h->max = h->max * MUL / DIV;
    h->high = high0 + h->max - max0;
    h = realloc(h, sizeof(header) + h->max * unit);
    if (high0 == max0) return h+1;
    char *b = (char *)(h + 1);
    memmove(b + h->high * unit, b + high0 * unit, (max0 - high0) * unit);
    return h + 1;
}

void *resize(void *a, int n) {
    a = ensure(a, n - length(a));
    setLength(a, n);
    return a;
}

void *adjust(void *a, int d) {
    a = ensure(a, d);
    setLength(a, length(a) + d);
    return a;
}

void clear(void *a) {
    header *h = (header *) a - 1;
    h->length = 0;
    h->high = h->max;
}

int high(void *a) {
    header *h = (header *) a - 1;
    return h->high;
}

int max(void *a) {
    header *h = (header *) a - 1;
    return h->max;
}

void *setHigh(void *a, int n) {
    a = ensure(a, high(a) - n);
    header *h = (header *) a - 1;
    h->high = n;
    return a;
}

// Move the gap from length(a) to n.
void moveGap(void *a, int n) {
    header *h = (header *) a - 1;
    int length0 = h->length, high0 = h->high, unit = h->unit;
    h->length = n;
    h->high = high0 - length0 + n;
    char *b = a;
    if (n < length0) {
        memmove(b + h->high * unit, b + n * unit, (length0 - n) * unit);
    }
    else {
        memmove(b + length0 * unit, b + high0 * unit, (n - length0) * unit);
    }
}

void error(char const *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void check(bool ok, char const *fmt, ...) {
    if (ok) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void *warn(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    return NULL;
}

// ---------- Testing ----------------------------------------------------------
#ifdef arrayTest

// Test arrays of characters.
static void testC() {
    char *a = newArray(1);
    a = resize(a, 10);
    for (int i = 0; i < 10; i++) a[i] = 42 + i;
    assert(length(a) == 10);
    a = ensure(a, 100);
    assert(length(a) == 10);
    assert(a[0] == 42 && a[9] == 51);
    a[99] = 0;
    adjust(a, -5);
    assert(length(a) == 5);
    assert(a[0] == 42 && a[4] == 46);
    freeArray(a);
}

// Test arrays of integers.
static void testI() {
    int *a = newArray(sizeof(int));
    a = resize(a, 10);
    for (int i = 0; i < 10; i++) a[i] = 42 + i;
    assert(length(a) == 10);
    a = ensure(a, 100);
    assert(length(a) == 10);
    assert(a[0] == 42 && a[9] == 51);
    a[99] = 0;
    adjust(a, -5);
    assert(length(a) == 5);
    assert(a[0] == 42 && a[4] == 46);
    freeArray(a);
}

// Test gap buffers of integers.
static void testG() {
    int *a = newArray(sizeof(int));
    a = ensure(a, 10);
    assert(max(a) == 13);
    a = resize(a, 5);
    for (int i = 0; i < 5; i++) a[i] = 42 + i;
    moveGap(a, 1);
    assert(length(a) == 1 && high(a) == 9);
    assert(a[0] == 42 && a[9] == 43 && a[10] == 44 && a[12] == 46);
    a = ensure(a, 9);
    assert(max(a) == 19);
    assert(a[0] == 42 && a[15] == 43 && a[16] == 44 && a[18] == 46);
    freeArray(a);
}

int main() {
    testC();
    testI();
    testG();
    printf("Array module OK\n");
}

#endif
