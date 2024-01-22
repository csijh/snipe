// The Snipe editor is free and open source. See licence.txt.
#include "text.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// The text and kinds are stored in synchronized gap buffers. Gap buffers are
// difficult to beat for simplicity and efficiency:
//     https://coredumped.dev/2023/08/09/text-showdown-gap-buffers-vs-ropes/
struct text { int low, high, max; char *chars; byte *kinds; };

// Initial size and expansion factor.
enum { MAX0 = 2, MUL = 3, DIV = 2 };

Text *newText() {
    Text *t = malloc(sizeof(Text));
    char *chars = malloc(MAX0);
    byte *kinds = malloc(MAX0);
    *t = (Text) { .low=0, .high=MAX0, .max=MAX0, .chars=chars, .kinds=kinds };
    return t;
}

void freeText(Text *t) {
    free(t->chars);
    free(t->kinds);
    free(t);
}

static void ensureT(Text *t, int extra) {
    int low = t->low, high = t->high, max = t->max;
    char *chars = t->chars;
    byte *kinds = t->kinds;
    int new = max;
    while (new < low + max - high + extra) new = new * MUL / DIV;
    chars = realloc(chars, new);
    kinds = realloc(kinds, new);
    if (high < max) {
        memmove(chars + high + new - max, chars + high, max - high);
        memmove(kinds + high + new - max, kinds + high, max - high);
    }
    t->high = high + new - max;
    t->max = new;
    t->chars = chars;
    t->kinds = kinds;
}

void load(Text *t, char *path) {
    int size = sizeFile(path);
    if (size < 0) {
        printf("Unable to load file '%s'.\n", path);
        return;
    }
    t->low = 0;
    t->high = t->max;
    ensureT(t, size);
}

int lengthT(Text *t) {
    return t->low + t->max - t->high;
}

char getT(Text *t, int i) {
    if (i < t->low) return t->chars[i];
    return t->chars[i + t->high - t->low];
}

byte getK(Text *t, int i) {
    if (i < t->low) return t->kinds[i];
    return t->kinds[i + t->high - t->low];
}

void setT(Text *t, int i, char c) {
    if (i < t->low) t->chars[i] = c;
    else t->chars[i + t->high - t->low] = c;
}

void setK(Text *t, int i, byte k) {
    if (i < t->low) t->kinds[i] = k;
    else t->kinds[i + t->high - t->low] = k;
}

void moveT(Text *t, int cursor) {
    int low = t->low, high = t->high;
    char *chars = t->chars;
    byte *kinds = t->kinds;
    if (cursor < low) {
        memmove(chars + cursor + high - low, chars + cursor, low - cursor);
        memmove(kinds + cursor + high - low, kinds + cursor, low - cursor);
    }
    else if (cursor > low) {
        memmove(chars + low, chars + high, cursor - low);
        memmove(kinds + low, kinds + high, cursor - low);
    }
    t->low = cursor;
    t->high = cursor + high - low;
}

void insertT(Text *t, int i, char *s, int n) {
    if (t->high - t->low < n) ensureT(t, n);
    moveT(t, i);
    memcpy(t->chars + t->low, s, n);
    memset(t->kinds + t->low, More, n);
    t->low += n;
}

void deleteT(Text *t, int i, char *s, int n) {
    moveT(t, i + n);
    memcpy(s, t->chars + i, n);
    t->low = i;
}

void copyT(Text *t, int i, char *s, int n) {
    moveT(t, i + n);
    memcpy(s, t->chars + i, n);
}

void copyK(Text *t, int i, byte *s, int n) {
    moveT(t, i + n);
    memcpy(s, t->kinds + i, n);
}

int cursorT(Text *t) {
    return t->low;
}

// Load a file (deleting any previous content) or save the content into a file.
void load(Text *t, char *path);
void save(Text *t, char *path);

// ---------- Testing ----------------------------------------------------------
#ifdef textTest

// Check that a Text object matches a string.
static bool eq(Text *t, char *s) {
    if (strlen(s) != t->max) return false;
    for (int i = 0; i < t->max; i++) {
        if (t->low <= i && i < t->high) { if (s[i] != '-') return false; }
        else if (s[i] != t->chars[i]) return false;
    }
    return true;
}

// Test gap buffer with char items.
static void test() {
    Text *t = newText();
    ensureT(t, 10);
    assert(eq(t, "-------------"));
    insertT(t, 0, "abcde", 5);
    assert(eq(t, "abcde--------"));
    moveT(t, 2);
    assert(eq(t, "ab--------cde"));
    char out[10];
    deleteT(t, 1, out, 1);
    assert(eq(t, "a---------cde"));
    assert(out[0] == 'b');
    ensureT(t, 14);
    assert(eq(t, "a---------------cde"));
    moveT(t,3);
    assert(eq(t, "acd---------------e"));
    insertT(t, 3, "xyz", 3);
    assert(eq(t, "acdxyz------------e"));
    insertT(t, 1, "uvw", 3);
    assert(eq(t, "auvw---------cdxyze"));
    freeText(t);
}

int main() {
    test();
    printf("Text module OK\n");
}

#endif
