// The Snipe editor is free and open source. See licence.txt.
#include "text.h"
#include "file.h"
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// The characters and styles of a text file are in synchronized gap buffers.
struct text { char *chars; byte *styles; };

Text *newText() {
    Text *t = malloc(sizeof(Text));
    char *chars = newArray(sizeof(char));
    byte *styles = newArray(sizeof(byte));
    *t = (Text) { .chars=chars, .styles=styles };
    return t;
}

void freeText(Text *t) {
    freeArray(t->chars);
    freeArray(t->styles);
    free(t);
}

void load(Text *t, char *path) {
    clear(t->chars);
    clear(t->styles);
    t->chars = readFile(path, t->chars);
    // clean
    // lines
}
/*
//----------------------------------------
int lengthT(Text *t) {
    return t->low + t->max - t->high;
}

char getT(Text *t, int i) {
    if (i < t->low) return t->chars[i];
    return t->chars[i + t->high - t->low];
}

byte getK(Text *t, int i) {
    if (i < t->low) return t->styles[i];
    return t->styles[i + t->high - t->low];
}

void setT(Text *t, int i, char c) {
    if (i < t->low) t->chars[i] = c;
    else t->chars[i + t->high - t->low] = c;
}

void setK(Text *t, int i, byte k) {
    if (i < t->low) t->styles[i] = k;
    else t->styles[i + t->high - t->low] = k;
}

void moveT(Text *t, int cursor) {
    int low = t->low, high = t->high;
    char *chars = t->chars;
    byte *styles = t->styles;
    if (cursor < low) {
        memmove(chars + cursor + high - low, chars + cursor, low - cursor);
        memmove(styles + cursor + high - low, styles + cursor, low - cursor);
    }
    else if (cursor > low) {
        memmove(chars + low, chars + high, cursor - low);
        memmove(styles + low, styles + high, cursor - low);
    }
    t->low = cursor;
    t->high = cursor + high - low;
}

void insertT(Text *t, int i, char *s, int n) {
    if (t->high - t->low < n) ensureT(t, n);
    moveT(t, i);
    memcpy(t->chars + t->low, s, n);
    memset(t->styles + t->low, None, n);
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
    memcpy(s, t->styles + i, n);
}

int cursorT(Text *t) {
    return t->low;
}

// Load a file (deleting any previous content) or save the content into a file.
void load(Text *t, char *path);
void save(Text *t, char *path);
*/
// ---------- Testing ----------------------------------------------------------
#ifdef textTest

/*
// Check that a Text object matches a string.
static bool eq(Text *t, char *s) {
    if (strlen(s) != t->max) return false;
    for (int i = 0; i < t->max; i++) {
        if (t->low <= i && i < t->high) { if (s[i] != '-') return false; }
        else if (s[i] != t->chars[i]) return false;
    }
    return true;
}
*/
// Test gap buffer with char items.
static void test() {
    Text *t = newText();
    load(t, "./12.txt");
    printf("%.8s", t->chars);
/*
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
    */
    freeText(t);
}

int main() {
    test();
    printf("Text module OK\n");
}

#endif
