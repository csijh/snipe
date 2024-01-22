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

static void setLength(void *a, int n) {
    header *h = (header *) a - 1;
    h->length = n;
}

void *padTo(void *a, int n) {
    header *h = (header *) a - 1;
    if (h->max >= n) return a;
    while (h->max < n) h->max = h->max * MUL / DIV;
    h = realloc(h, sizeof(header) + h->max * h->unit);
    return h + 1;
}

void *padBy(void *a, int d) {
    return padTo(a, length(a) + d);
}

void *adjustTo(void *a, int n) {
    a = padTo(a, n);
    setLength(a, n);
    return a;
}

void *adjustBy(void *a, int d) {
    a = padBy(a, d);
    setLength(a, length(a) + d);
    return a;
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

static void testC() {
    char *a = newArray(1);
    a = adjustBy(a, 10);
    for (int i = 0; i < 10; i++) a[i] = 42 + i;
    assert(length(a) == 10);
    a = padBy(a, 100);
    assert(length(a) == 10);
    assert(a[0] == 42 && a[9] == 51);
    a[99] = 0;
    adjustBy(a, -5);
    assert(length(a) == 5);
    assert(a[0] == 42 && a[4] == 46);
    freeArray(a);
}

static void testI() {
    int *a = newArray(sizeof(int));
    a = adjustBy(a, 10);
    for (int i = 0; i < 10; i++) a[i] = 42 + i;
    assert(length(a) == 10);
    a = padBy(a, 100);
    assert(length(a) == 10);
    assert(a[0] == 42 && a[9] == 51);
    a[99] = 0;
    adjustBy(a, -5);
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
