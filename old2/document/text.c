// The Snipe editor is free and open source, see licence.txt.
#include "text.h"
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// TODO: text -> cursors -> history
// Who keeps track of cursors which hold trailers? Can text ask?
// After action: (a) collapse (b) check trailers

// A text object stores an array of bytes, as a gap buffer, and an array of
// cursors. The gap is between offsets lo and hi in the data array. After each
// edit, startEdit and endEdit cover the range of text which may have changed.
// For realloc info, see
// http://blog.httrack.com/blog/2014/04/05/a-story-of-realloc-and-laziness/
struct text {
    char *data;
    int lo, hi, end;
    cursors *cs;
    history *h;
    int startEdit, endEdit;
};

text *newText(cursors *cs, history *h) {
    int n = 1024;
    text *t = malloc(sizeof(text));
    char *data = malloc(n);
    *t = (text) { .lo=0, .hi=n, .end=n, .data=data, .cs=cs, .h=h };
    t->startEdit = -1;
    t->endEdit = -1;
    return t;
}

void freeText(text *t) {
    free(t->data);
    free(t);
}

extern inline int lengthText(text *t) {
    return t->lo + (t->end - t->hi);
}

// Resize to make room for an insertion of n bytes.
static void resizeText(text *t, int n) {
    int hilen = t->end - t->hi;
    int needed = t->lo + n + hilen;
    int size = t->end;
    if (size >= needed) return;
    while (size < needed) size = size * 3 / 2;
    t->data = realloc(t->data, size);
    if (hilen > 0) memmove(&t->data[size - hilen], &t->data[t->hi], hilen);
    t->hi = size - hilen;
    t->end = size;
}

// Move the gap to the given position.
static void moveGap(text *t, position at) {
    if (at < 0) at = 0;
    if (at > lengthText(t)) at = lengthText(t);
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
/*
// Expand the fix range to cover an insertion or deletion, plus one extra byte,
// so covering any byte which might contain a newline with preceding spaces.
static void addRange(text *t, int from, int to) {
    if (t->startFix < 0) t->startFix = t->endFix = from;
    if (from < t->startFix) t->startFix = from;
    if (to >= t->endFix) {
        if (to < lengthText(t)) t->endFix = to + 1;
        else t->endFix = to;
    }
}

// Clean up new text, assumed to be UTF-8 valid. Remove \0 to \7, carriage
// returns, trailing spaces, trailing blank lines, and ensure a final newline.
static int clean(int n, char *s) {
    int j = 0;
    for (int i = 0; i < n; i++) {
        char ch = s[i];
        if ('\0' <= ch && ch <= '\7') continue;
        if (ch == '\r') continue;
        if (ch == '\n') {
            while (j > 0 && s[j-1] == ' ') j--;
        }
        s[j++] = ch;
    }
    n = j;
    while (n > 0 && s[n-1] == ' ') n--;
    if (n > 0 && s[n-1] != '\n') s[n++] = '\n';
    while (n > 1 && s[n-2] == '\n') n--;
    s[n] = '\0';
    return n;
}

bool loadText(text *t, int n, char *buffer) {
    bool ok = uvalid(n, buffer, true);
    if (! ok) return false;
    n = clean(n, buffer);
    t->lo = 0;
    t->hi = t->end;
    resizeText(t, n);
    t->hi = t->end - n;
    memcpy(&t->data[t->hi], buffer, n);
    return true;
}

void getText(text *t, position at, int n, char *s) {
    if (at + n > lengthText(t)) return;
    moveGap(t, at + n);
    memcpy(s, &t->data[at], n);
    s[n] = '\0';
}

// Update the fix range after an insertion (n>0) or deletion (n<0). Cover the
// case where the position was in the middle of the deleted text.
static void update(text *t, int at, int n, bool insert) {
    if (insert) {
        if (t->startFix > at) t->startFix += n;
        if (t->endFix >= at) t->endFix += n;
    }
    else {
        if (t->startFix > at + n) t->startFix -= n;
        else if (t->startFix >= at) t->startFix = at;
        if (t->endFix > at + n) t->endFix -= n;
        else if (t->endFix > at) t->endFix = at;
    }
}

// Carry out an insertion.
static void insertText(text *t, edit *e) {
    int at = atEdit(e);
    moveGap(t, at);
    int n = lengthEdit(e);
    if (n > t->hi - t->lo) resizeText(t, n);
    copyEdit(e, &t->data[at]);
    t->lo = t->lo + n;
    update(t, at, n, true);
    addRange(t, at, at + n);
}

//static char * show(text *t, char *s);
// Carry out a deletion.
static void deleteText(text *t, edit *e) {
    int at = atEdit(e);
    int n = lengthEdit(e);
    moveGap(t, at + n);
    t->lo = t->lo - n;
    update(t, at, n, false);
    addRange(t, at, at);
//char temp[100];
//show(t, temp);
//printf("t=<%s>\n", temp);
//printf("f=%d %d\n", t->startFix, t->endFix);
}

void editText(text *t, edit *e) {
//printf("op=%d\n", opEdit(e));
    switch (opEdit(e)) {
        case DoInsert: insertText(t, e); break;
        case DoDelete: deleteText(t, e); break;
        case DoEnd: break;
        default: moveGap(t, toEdit(e)); break;
    }
}

// Look for the first trailing spaces between startFix and endFix, i.e. a
// newline with preceding spaces.
static bool fixTrailingSpaces(text *t, edit *e) {
    moveGap(t, t->endFix);
    int i;
    for (i = t->startFix; i < t->endFix; i++) {
        if (t->data[i] == '\n' && i > 0 && t->data[i-1] == ' ') break;
    }
    t->startFix = i;
    if (i == t->endFix) return false;
    int from = i, to = i;
    while (from > 0 && t->data[from-1] == ' ') from--;
    deleteEdit(e, from, to-from, &t->data[from]);
    return true;
}

// Check for just one newline at the end of the text.
bool fixEndText(text *t, edit *e) {
    t->startFix = t->endFix = -1;
    int n = lengthText(t);
    moveGap(t, n);
    if (n > 0 && t->data[n-1] != '\n') {
        insertEdit(e, n, 1, "\n");
        return true;
    }
    if (n > 1 && t->data[n-2] == '\n') {
        int from = n, to = n;
        while (from > 1 && t->data[from-2] == '\n') from--;
        deleteEdit(e, n, to-from, &t->data[from]);
        return true;
    }
    return false;
}

// Look for trailing spaces between startFix and endFix. If the two are equal,
// check for trailing blank lines or a missing final newline. If the two are -1,
// there are no further fixes.
bool fixText(text *t, edit *e) {
    if (t->startFix < t->endFix && fixTrailingSpaces(t, e)) return true;
    if (t->startFix >= 0) return fixEndText(t, e);
    return false;
}
*/
// Unit testing
#ifdef textTest
/*
// Flatten the text into the given string.
static char *show(text *t, char *s) {
    int j = 0;
    for (int i = 0; i < t->end; i++) {
        if (i < t->lo || i >= t->hi) s[j++] = t->data[i];
    }
    s[j] = '\0';
    return s;
}

// Compare text object against pattern.
static bool compare(text *t, char *p) {
    char actual[100];
    show(t, actual);
//    printf("a=%s\n", actual);
    return strcmp(actual, p) == 0;
}

// Test an insertion followed by all its fixes. The 'before' pattern contains
// [...] to indicate the inserted text.
static bool testInsert(char *before, char *after) {
    char temp[100];
    strcpy(temp, before);
    int open = strchr(temp, '[') - temp;
    int close = strchr(temp, ']') - temp;
    int len = strlen(temp);
    text *t = newText();
    memcpy(t->data, temp, open);
    memcpy(&t->data[open], &temp[close+1], len - close - 1);
    t->lo = open + len - close - 1;
    edit *e = newEdit();
    insertEdit(e, open, close-open-1, &temp[open+1]);
    editText(t, e);
    while (fixText(t, e)) editText(t, e);
    bool ok = compare(t, after);
    freeEdit(e);
    freeText(t);
    return ok;
}

// Test a deletion followed by all its fixes. The 'before' pattern contains
// [...] to indicate the deleted text.
static bool testDelete(char *before, char *after) {
    char temp[100];
    strcpy(temp, before);
    int open = strchr(temp, '[') - temp;
    int close = strchr(temp, ']') - temp;
    int len = strlen(temp);
    text *t = newText();
    memcpy(t->data, temp, open);
    memcpy(&t->data[open], &temp[open+1], close-open-1);
    memcpy(&t->data[close-1], &temp[close+1], len-close-1);
    t->lo = len - 2;
    edit *e = newEdit();
    deleteEdit(e, open, close-open-1, &temp[open+1]);
    editText(t, e);
    while (fixText(t, e)) editText(t, e);
    bool ok = compare(t, after);
    freeEdit(e);
    freeText(t);
    return ok;
}
*/
// Test all the ways in which trailing spaces, trailing blank lines or missing
// final newlines can occur through an insertion or deletion.
int main() {
    /*
    assert(testInsert("abc[def]ghi\n", "abcdefghi\n"));
    assert(testInsert("x  [\ny\n]z\n", "x\ny\nz\n")); // cursor holds trailers??
    assert(testInsert("x[\ny  \n]z\n", "x\ny\nz\n")); // clean
    assert(testInsert("x[\ny\nz  ]\n", "x\ny\nz\n")); // clean
    assert(testInsert("x[\ny\nz]", "x\ny\nz\n"));     // clean (insert more)
    assert(testInsert("x[\ny\nz  ]", "x\ny\nz\n"));   // clean (del/ins)
    assert(testInsert("x[\ny\nz\n]\n", "x\ny\nz\n")); // clean
    assert(testInsert("x[\ny\nz\n\n]", "x\ny\nz\n")); // clean
    assert(testInsert("x\ny\nz\n[\n]", "x\ny\nz\n")); // clean

    assert(testDelete("abc[def]ghi\n", "abcghi\n"));
    assert(testDelete("x\n  [y]\nz\n", "x\n\nz\n"));  // cursor holds trailers
    assert(testDelete("x\ny\n[z]\n", "x\ny\n"));      // clean (del more?)
    assert(testDelete("x\ny[\nz\n]", "x\ny\n"));      // clean (del less?)
*/
    printf("Text module OK\n");
    return 0;
}

#endif
