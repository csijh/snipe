// The Snipe editor is free and open source. See licence.txt.
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

// Pointer-aligned header, prefixed to arrays.
struct header { int length, max, unit; void *align[]; };
typedef struct header header;

// Start small and multiply size by 1.5:
enum { MAX0 = 2, MUL = 3, DIV = 2 };

void *newArray(int unit) {
    header *h = malloc(sizeof(header) + MAX0 * unit);
    *h = (header) { .length = 0, .max = MAX0, .unit = unit };
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

void *ensure(void *a, int d) {
    header *h = (header *) a - 1;
    if (h->max >= h->length + d) return h + 1;
    while (h->max < h->length + d) h->max = h->max * MUL / DIV;
    h = realloc(h, sizeof(header) + h->max * h->unit);
    return h + 1;
}

void *adjust(void *a, int d) {
    a = ensure(a, d);
    header *h = (header *) a - 1;
    h->length += d;
    return h + 1;
}

// Change length to n.
void *resize(void *a, int n) {
    a = ensure(a, n - length(a));
    header *h = (header *) a - 1;
    h->length = n;
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

static void testC() {
    char *a = newArray(1);
    a = adjust(a, 10);
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

static void testI() {
    int *a = newArray(sizeof(int));
    a = adjust(a, 10);
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

// Test all the ways in which trailing spaces, trailing blank lines or missing
// final newlines can occur through an insertion or deletion.
int main() {
    testC();
    testI();
    printf("Array module OK\n");
}

#endif
