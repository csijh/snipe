// The Snipe editor is free and open source, see licence.txt.
#include "gap.h"
#include "string.h"
#include "file.h"
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// A text object stores an array of bytes, as a gap buffer. For realloc info,
// see http://blog.httrack.com/blog/2014/04/05/a-story-of-realloc-and-laziness/
// The bytes are stored in data and the gap is between offsets lo and hi.
struct text {
    char *data;
    int lo, hi, top;
};

// Create an empty array with a small capacity.
text *newText() {
    int n = 24;
    text *t = malloc(sizeof(text));
    char *data = malloc(n);
    *t = (text) { .lo = 0, .hi = n, .top = n, .data = data };
    return t;
}

void freeText(text *t) {
    free(t->data);
    free(t);
}

int lengthText(text *t) {
    return t->lo + t->top - t->hi;
}

// Resize to make room for an insertion of n bytes.
static void resizeText(text *t, int n) {
    int hilen = t->top - t->hi;
    int needed = t->lo + n + hilen;
    int size = t->top;
    while (size < needed) size = size * 3 / 2;
    t->data = realloc(t->data, size);
    memmove(&t->data[size - hilen], &t->data[t->hi], hilen);
    t->hi = size - hilen;
    t->top = size;
}

// Move the gap to the given position.
static void moveGap(text *t, int at) {
    assert(at <= lengthText(t));
    if (at < t->lo) {
        int len = (t->lo - at);
        memmove(&t->data[t->hi - len], &t->data[at], len);
        t->hi = t->hi - len;
        t->lo = at;
    }
    else if (at > t->lo) {
        int len = (at - t->lo);
        memmove(&t->data[t->lo], &t->data[t->hi], len);
        t->hi = t->hi + len;
        t->lo = at;
    }
}

void getText(text *t, int at, int n, chars *s) {
    assert(at + n <= lengthText(t));
    moveGap(t, at + n);
    resize(s, n);
    memcpy(C(s), &t->data[at], n);
}

// Insert s at position at.
void insertText(text *t, int at, int n, char const s[n]) {
    moveGap(t, at);
    if (n > t->hi - t->lo) resizeText(t, n);
    memcpy(&t->data[t->lo], s, n);
    t->lo = t->lo + n;
}

// Delete n bytes at position at, and handle the side effects.
// Move to the nearest end of the deletion, in case n is very large.
void deleteText(text *t, int at, int n) {
    if (t->lo < at + n / 2) {
        moveGap(t, at + n);
        t->lo = at;
    }
    else {
        moveGap(t, at);
        t->hi = t->hi + n;
    }
}

void changeText(text *t, op *o) {
    int flags = flagsOp(o);
    int at = atOp(o);
    int n = lengthOp(o);
    char *s = textOp(o);
    if (flags & Del) deleteText(t, at, n);
    else insertText(t, at, n, s);
}

static text *emptyText() {
    int size = 24;
    char *data = malloc(size);
    strcpy(data, "\n");
    text *t = malloc(sizeof(text));
    *t = (text) { .lo = 1, .hi = size, .top = size, .data = data };
    return t;
}

static void err(char const *e, char const *p) {
    printf("Error, %s: %s\n", e, p);
}

text *readText(char const *path) {
    char *data = readPath(path);
    if (data == NULL) return emptyText();
    int size = strlen(data);
    char const *message = utf8valid(data, size);
    if (message != NULL) { err(message, path); free(data); return emptyText(); }
    size = normalize(data);
    text *t = malloc(sizeof(text));
    *t = (text) { .lo = size, .hi = size, .top = size, .data = data };
    return t;
}

void writeText(text *t, char const *path) {
    int size = lengthText(t);
    moveGap(t, size);
    writeFile(path, size, t->data);
}

// Unit testing
#ifdef gapTest

// Compare text object against pattern with ... as the gap.
static bool compare(text *t, char *p) {
    char *gap = strstr(p, "...");
    int n = gap - p;
    if (n != t->lo) return false;
    if (memcmp(p, t->data, n) != 0) return false;
    p = gap + 3;
    n = strlen(p);
    if (n != t->top - t->hi) return false;
    if (memcmp(p, &t->data[t->hi], n) != 0) return false;
    return true;
}

int main() {
    setbuf(stdout, NULL);
    text *t = newText();
    assert(compare(t, "..."));
    insertText(t, 0, 6, "abcdz\n");
    assert(compare(t, "abcdz\n..."));
    insertText(t, 4, 21, "efghijklmnopqrstuvwxy");
    assert(compare(t, "abcdefghijklmnopqrstuvwxy...z\n"));
    moveGap(t, 5);
    assert(compare(t, "abcde...fghijklmnopqrstuvwxyz\n"));
    deleteText(t, 4, 4);
    assert(compare(t, "abcd...ijklmnopqrstuvwxyz\n"));
    deleteText(t, 0, 7);
    assert(compare(t, "...lmnopqrstuvwxyz\n"));
    deleteText(t, 0, 16);
    assert(compare(t, "..."));
    insertText(t, 0, 9, "a\nbb\nccc\n");
    assert(compare(t, "a\nbb\nccc\n..."));
    deleteText(t, 3, 3);
    assert(compare(t, "a\nb...cc\n"));
    insertText(t, 3, 3, "b\nc");
    assert(compare(t, "a\nbb\nc...cc\n"));
    freeText(t);
    printf("Text module OK\n");
    return 0;
}

#endif
