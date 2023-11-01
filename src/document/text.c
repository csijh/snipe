// Snipe text handling. Free and open source, see licence.txt.
#include "text.h"
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// A text object stores an array of bytes, as a gap buffer. The gap is between
// offsets lo and hi in the data array. For realloc info, see
// http://blog.httrack.com/blog/2014/04/05/a-story-of-realloc-and-laziness/
struct text {
    char *data;
    int lo, hi, end;
};

edit *newEdit(int n, edit *old) {
    if (old != NULL && old->max > n) return old;
    edit *e = realloc(old, sizeof(edit) + n + 1);
    e->at = e->to = e->n = 0;
    e->max = n;
    return e;
}

text *newText() {
    int n = 1024;
    text *t = malloc(sizeof(text));
    char *data = malloc(n);
    *t = (text) { .lo=0, .hi=n, .end=n, .data=data };
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
static void moveGap(text *t, int at) {
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

// Clean up new text, assumed to be UTF-8 valid. Normalise line endings, and
// remove internal trailing spaces.
static int clean(int n, char *s) {
    int j = 0;
    for (int i = 0; i < n; i++) {
        int ch = s[i];
        if (ch == 0xE2 && (int)s[i+1] == 0x80 && (int)s[i+2] == 0xA8) {
            ch = '\n';
            i = i + 2;
        }
        else if (ch == 0xE2 && (int)s[i+1] == 0x80 && (int)s[i+2] == 0xA9) {
            ch = '\n';
            i = i + 2;
        }
        else if (ch == '\r' && s[i+1] != '\n') {
            ch = '\n';
            i = i + 1;
        }
        else if (ch == '\r') continue;
        else if (ch == '\n') {
            while (j > 0 && s[j-1] == ' ') j--;
        }
        s[j++] = ch;
    }
    n = j;
    s[n] = '\0';
    return n;
}

// Clean the buffer, and also remove any final trailing spaces or blank lines,
// and ensure a final newline. Then copy the buffer as the new text.
bool loadText(text *t, int n, char *buffer) {
    bool ok = uvalid(n, buffer);
    if (! ok) return false;
    n = clean(n, buffer);
    while (n > 0 && buffer[n-1] == ' ') n--;
    if (n > 0 && buffer[n-1] != '\n') buffer[n++] = '\n';
    while (n > 1 && buffer[n-2] == '\n') n--;
    t->lo = 0;
    t->hi = t->end;
    resizeText(t, n);
    t->hi = t->end - n;
    memcpy(&t->data[t->hi], buffer, n);
    return true;
}

// Insert text. As well as cleaning the text, adjust it to avoid creating
// trailing spaces, blank lines or a missing final newline in context.
edit *insertText(text *t, edit *e) {
    int len = lengthText(t);
    assert(0 <= e->at && e->at <= e->to && e->to <= len && e->n >= 0);
    moveGap(t, e->at);
    int n = clean(e->n, e->s);
    if (e->at == len) {
        while (n > 0 && e->s[n-1] == ' ') n--;
        if (n > 0 && e->s[n-1] != '\n') e->s[n++] = '\n';
        while (n > 1 && e->s[n-2] == '\n') n--;
        if (n == 1 && e->s[0] == '\n') n--;
    }
    else if (e->at == len - 1) {
        while (n > 0 && e->s[n-1] == ' ') n--;
        while (n > 0 && e->s[n-1] == '\n') n--;
    }
    else if (t->data[t->hi] == '\n') {
        while (n > 0 && e->s[n-1] == ' ') n--;
    }
    e->n = n;
    e->to = e->at;
    if (n > 0 && e->s[0] == '\n') {
        while (e->at > 0 && t->data[e->at - 1] == ' ') e->at--;
    }
    t->lo = e->at;
    if (n > t->hi - t->lo) resizeText(t, n);
    memcpy(&t->data[t->lo], e->s, n);
    t->lo = t->lo + n;
    return e;
}

// Delete a edit. Clean it, and adjust, possibly at both ends, to avoid
// creating trailing spaces, blank lines or a missing final newline in context.
edit *deleteText(text *t, edit *e) {
    int len = lengthText(t);
    assert(0 <= e->to && e->to < e->at && e->at <= len);
    moveGap(t, e->at);
    if (e->at > 0 && e->at == len) {
        e->at--;
        moveGap(t, e->at);
    }
    if (e->at == len - 1) {
        while (e->to > 0 && t->data[e->to - 1] == '\n') e->to--;
    }
    if (t->data[t->hi] == '\n') {
        while (e->to > 0 && t->data[e->to - 1] == ' ') e->to--;
    }
    int n = e->at - e->to;
    t->lo = t->lo - n;
    if (n > e->max) e = newEdit(n, e);
    memcpy(e->s, &t->data[t->lo], n);
    return e;
}

edit *getText(text *t, edit *e) {
    int n = e->to - e->at;
    if (n > e->max) e = newEdit(n, e);
    moveGap(t, e->at);
    memcpy(e->s, &t->data[e->at], n);
    e->s[n] = '\0';
    return e;
}

// Unit testing
#ifdef textTest

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

// Test an insertion and its adjustments. The 'before' pattern contains
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
    edit *e = newEdit(100, NULL);
    e->at = e->to = open;
    e->n = close - 1 - open;
    memcpy(e->s, temp + open + 1, e->n);
    e = insertText(t, e);
    bool ok = compare(t, after);
    free(e);
    freeText(t);
    return ok;
}

// Test a deletion and its adjustments. The 'before' pattern contains
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
    edit *e = newEdit(100, NULL);
    e->to = open;
    e->at = close - 1;
    e = deleteText(t, e);
    bool ok = compare(t, after);
    free(e);
    freeText(t);
    return ok;
}

// Test all the ways in which trailing spaces, trailing blank lines or missing
// final newlines can occur through an insertion or deletion.
int main() {
    assert(testInsert("abc[def]ghi\n", "abcdefghi\n"));
    assert(testInsert("x[\ny  \n]z\n", "x\ny\nz\n")); // internal trailers
    assert(testInsert("x[\ny\nz  ]\n", "x\ny\nz\n")); // trailers at end
    assert(testInsert("x[\ny\nz]", "x\ny\nz\n"));     // final newline
    assert(testInsert("x[\ny\nz  ]", "x\ny\nz\n"));   // final trailers+newline
    assert(testInsert("x[\ny\nz\n]\n", "x\ny\nz\n")); // final blanks
    assert(testInsert("x[\ny\nz\n\n]", "x\ny\nz\n")); // final blanks
    assert(testInsert("x\ny\nz\n[\n]", "x\ny\nz\n")); // final blanks
    assert(testInsert("x  [\ny\n]z\n", "x\ny\nz\n")); // trailers at start

    assert(testDelete("abc[def]ghi\n", "abcghi\n"));
    assert(testDelete("x\n  [y]\nz\n", "x\n\nz\n"));  // trailers at start
    assert(testDelete("x\ny\n[z]\n", "x\ny\n"));      // final blanks
    assert(testDelete("x\ny[\nz\n]", "x\ny\n"));      // final newline

    printf("Text module OK\n");
    return 0;
}

#endif
