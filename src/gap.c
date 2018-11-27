// The Snipe editor is free and open source, see licence.txt.
#include "gap.h"
#include "list.h"

// A text object stores an array of bytes, as a gap buffer. For realloc info,
// see http://blog.httrack.com/blog/2014/04/05/a-story-of-realloc-and-laziness/
// The bytes are stored in data and the gap is between offsets lo and hi.
struct text {
    char *data;
    int lo, hi, end;
};

// Create an empty array with a small capacity.
text *newText() {
    int n = 24;
    text *t = malloc(sizeof(text));
    char *data = malloc(n);
    *t = (text) { .lo = 0, .hi = n, .end = n, .data = data };
    return t;
}

void freeText(text *t) {
    free(t->data);
    free(t);
}

int lengthText(text *t) {
    return t->lo + t->end - t->hi;
}

// Resize to make room for an insertion of n bytes.
static void resizeText(text *t, int n) {
    int hilen = t->end - t->hi;
    int needed = t->lo + n + hilen;
    int size = t->end;
    while (size < needed) size = size * 3 / 2;
    t->data = realloc(t->data, size);
    memmove(&t->data[size - hilen], &t->data[t->hi], hilen);
    t->hi = size - hilen;
    t->end = size;
}

// Move the gap to the given position.
static void moveGap(text *t, int p) {
    assert(p <= lengthText(t));
    if (p < t->lo) {
        int len = (t->lo - p);
        memmove(&t->data[t->hi - len], &t->data[p], len);
        t->hi = t->hi - len;
        t->lo = p;
    }
    else if (p > t->lo) {
        int len = (p - t->lo);
        memmove(&t->data[t->lo], &t->data[t->hi], len);
        t->hi = t->hi + len;
        t->lo = p;
    }
}

void getText(text *t, int p, int n, chars *s) {
    assert(p + n <= lengthText(t));
    moveGap(t, p + n);
    resize(s, n);
    memcpy(C(s), &t->data[p], n);
}
