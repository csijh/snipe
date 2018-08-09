// The Snipe editor is free and open source, see licence.txt.
#include "text.h"
#include "line.h"
#include "string.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// A text object stores an array of bytes, as a gap buffer. For realloc info,
// see http://blog.httrack.com/blog/2014/04/05/a-story-of-realloc-and-laziness/
// The bytes are stored in data and the gap is between offsets lo and hi. The
// text object also looks after the lines, styles and cursors.
struct text {
    char *data;
    int lo, hi, end;
    ints *lines;
    chars *styles;
    cursors *cs;
};

static void err(char const *e, char const *p) {
    printf("Error, %s: %s\n", e, p);
}

// Create an empty array with a small capacity.
text *newText() {
    int n = 24;
    text *t = malloc(sizeof(text));
    char *data = malloc(n);
    *t = (text) { .lo = 0, .hi = n, .end = n, .data = data };
    t->lines = newInts();
    t->styles = newChars();
    t->cs = newCursors(t->lines, t->styles);
    return t;
}

void freeText(text *t) {
    if (t == NULL) return;
    free(t->data);
    freeInts(t->lines);
    freeChars(t->styles);
    freeCursors(t->cs);
    free(t);
}

ints *getLines(text *t) {
    return t->lines;
}

chars *getStyles(text *t) {
    return t->styles;
}

cursors *getCursors(text *t) {
    return t->cs;
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
    if (p > lengthText(t)) *((char *)NULL) = '!';
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

// Pretend there is an extra line containing just a newline.
void getText(text *t, int p, int n, chars *s) {
    resize(s, 0);
    if (p == lengthText(t) && n == 1) {
        add(s, '\n');
    }
    else {
        moveGap(t, p + n);
        insert(s, 0, n, &t->data[p]);
    }
}

// Update the list of lines when there is an insertion or deletion.
static void updateLines(text *t, int p, int n) {
    ints *xs = t->lines;
    int count = length(xs);
    for (int i = 0; i < count; i++) {
        if (get(xs, i) > p) {
            set(xs, i, get(xs, i) + n);
            if (get(xs, i) < p) set(xs, i, p);
        }
    }
}

// Insert extra lines when string s is inserted at position p.
static void insertLines(text *t, int p, char const *s) {
    ints *lines = t->lines;
    int count = 0, index = 0;
    for (int i = 0; i < strlen(s); i++) if (s[i] == '\n') count++;
    if (count == 0) return;
    int len = length(lines);
    while (index < len && get(lines, index) <= p) index++;
    for (int i = 0; i < strlen(s); i++) if (s[i] == '\n') {
        int n = p + i + 1;
        insert(lines, index++, 1, &n);
    }
}

// Invalidate styles as the result of an insertion or deletion by forgetting all
// the values from the start of the line containing p onwards.
static void invalidateStyles(text *t, int p) {
    p = startLine(t->lines, findRow(t->lines, p));
    if (p < length(t->styles)) resize(t->styles, p);
}

// Insert s at position p, and handle the side effects.
void insertText(text *t, int p, char const *s) {
    moveGap(t, p);
    int n = strlen(s);
    if (n > t->hi - t->lo) resizeText(t, n);
    memcpy(&t->data[t->lo], s, n);
    t->lo = t->lo + n;
    updateCursors(t->cs, p, n);
    updateLines(t, p, n);
    insertLines(t, p, s);
    invalidateStyles(t, p);
}

void insertAt(text *t, char const *s) {
    for (int i = 0; i < countCursors(t->cs); i++) {
        int p = cursorAt(t->cs, i);
        insertText(t, p, s);
    }
}

// Delete lines when n bytes are deleted at position p.
static void deleteLines(text *t, int p, int n) {
    ints *lines = t->lines;
    int count = 0, index = 0;
    int len = length(lines);
    while (index < len && get(lines, index) <= p) index++;
    for (int i = index; i < len && get(lines, i) <= p+n; i++) count++;
    if (count > 0) delete(lines, index, count);
}

// Delete n bytes at position p, and handle the side effects.
// Move to the nearest end of the deletion, in case n is very large.
void deleteText(text *t, int p, int n) {
    if (t->lo < p + n / 2) {
        moveGap(t, p + n);
        t->lo = p;
    }
    else {
        moveGap(t, p);
        t->hi = t->hi + n;
    }
    updateCursors(t->cs, p, -n);
    deleteLines(t, p, n);
    updateLines(t, p, -n);
    invalidateStyles(t, p);
}

void deleteAt(text *t) {
    for (int i = 0; i < countCursors(t->cs); i++) {
        int p = cursorAt(t->cs, i);
        int f = cursorFrom(t->cs, i);
        if (f < p) deleteText(t, f, p-f);
        else if (p < f) deleteText(t, p, f-p);
    }
}

static text *emptyText() {
    int size = 16;
    char *data = malloc(size);
    strcpy(data, "\n");
    text *t = malloc(sizeof(text));
    *t = (text) { .lo = 1, .hi = size, .end = size, .data = data };
    t->lines = newInts();
    t->styles = newChars();
    t->cs = newCursors(t->lines, t->styles);
    insertLines(t, 0, data);
    return t;
}

text *readText(char const *path) {
    char *data = readPath(path);
    if (data == NULL) return emptyText();
    int size = strlen(data);
    char const *message = utf8valid(data, size);
    if (message != NULL) { err(message, path); free(data); return emptyText(); }
    size = normalize(data);
    text *t = malloc(sizeof(text));
    *t = (text) { .lo = size, .hi = size + 1, .end = size + 1, .data = data };
    t->lines = newInts();
    t->styles = newChars();
    t->cs = newCursors(t->lines, t->styles);
    insertLines(t, 0, data);
    return t;
}

void writeText(text *t, char const *path) {
    int size = lengthText(t);
    moveGap(t, size);
    for (int i = 0; i < size; i++) if (t->data[i] == '\0') t->data[i] = '\n';
    writeFile(path, size, t->data);
}

// Unit testing
#ifdef test_text

// Compare text object against pattern with ... as the gap.
static bool compare(text *t, char *p) {
    char *gap = strstr(p, "...");
    int n = gap - p;
    if (n != t->lo) return false;
    if (memcmp(p, t->data, n) != 0) return false;
    p = gap + 3;
    n = strlen(p);
    if (n != t->end - t->hi) return false;
    if (memcmp(p, &t->data[t->hi], n) != 0) return false;
    return true;
}

static bool compareLines(text *t, char *p) {
    ints *lines = t->lines;
    int n = strlen(p);
    if (n != length(lines)) return false;
    for (int i = 0; i < strlen(p); i++) {
        if (get(lines, i) != (p[i] - '0')) return false;
    }
    return true;
}

int main() {
    setbuf(stdout, NULL);
    text *t = newText();
    insertText(t, 0, "abcdz");
    assert(compare(t, "abcdz..."));
    insertText(t, 4, "efghijklmnopqrstuvwxy");
    assert(compare(t, "abcdefghijklmnopqrstuvwxy...z"));
    moveGap(t, 5);
    assert(compare(t, "abcde...fghijklmnopqrstuvwxyz"));
    deleteText(t, 4, 4);
    assert(compare(t, "abcd...ijklmnopqrstuvwxyz"));
    deleteText(t, 0, 7);
    assert(compare(t, "...lmnopqrstuvwxyz"));
    deleteText(t, 0, 15);
    assert(compare(t, "..."));
    insertText(t, 0, "a\nbb\nccc\n");
    assert(compareLines(t, "259"));
    deleteText(t, 3, 3);
    assert(compare(t, "a\nb...cc\n"));
    assert(compareLines(t, "26"));
    insertText(t, 3, "b\nc");
    assert(compareLines(t, "259"));
    freeText(t);
    printf("Text module OK\n");
    return 0;
}
#endif
