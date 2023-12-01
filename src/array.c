// Snipe editor. Free and open source, see licence.txt.
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// Pointer-aligned header, prefixed to arrays.
struct header { int length, high, max, unit; void *align[]; };
typedef struct header header;

// There is no harm in starting sizes at 1 unit: all it means is that the first
// few reallocations may be in-place without relocation, so almost cost-free.
// For large arrays, doubling becomes less appropriate, because there can't be
// a rapid series of small increases coming from user actions. So instead,
// sizes are increased a page at a time.
enum { MAX0 = 1, PAGE = 4096 };

void *newArray(int unit) {
    header *h = malloc(sizeof(header) + MAX0 * unit);
    *h = (header) { .length = 0, .high = MAX0, .max = MAX0, .unit = unit };
    return h + 1;
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

void *ensure(void *a, int by) {
    header *h = (header *) a - 1;
    int hilen = (h->max - h->high);
    int needed = (h->length + by + hilen);
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

void *adjust(void *a, int by) {
    a = ensure(a, by);
    header *h = (header *) a - 1;
    h->length += by;
    if (h->length < 0) h->length = 0;
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
