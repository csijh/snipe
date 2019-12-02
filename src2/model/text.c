// The Snipe editor is free and open source, see licence.txt.
#include "text.h"
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// A text object stores an array of bytes, as a gap buffer. For realloc info,
// see http://blog.httrack.com/blog/2014/04/05/a-story-of-realloc-and-laziness/
// The gap is between offsets lo and hi in the data array.
struct text {
    char *data;
    int lo, hi, top;
};

// Visualize the text in the given string.
static char *show(text *t, char *s) {
    int j = 0;
    for (int i = 0; i < t->lo; i++) {
        char ch = t->data[i];
        if (ch == '\n') { s[j++] = '\\'; s[j++] = 'n'; }
        else s[j++] = ch;
    }
    s[j++] = '.'; s[j++] = '.'; s[j++] = '.';
    for (int i = t->hi; i < t->top; i++) {
        char ch = t->data[i];
        if (ch == '\n') { s[j++] = '\\'; s[j++] = 'n'; }
        else s[j++] = ch;
    }
    s[j] = '\0';
    return s;
}

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

// Resize to make room for an insertion of n bytes.
static void resizeText(text *t, length n) {
    int hilen = t->top - t->hi;
    int needed = t->lo + n + hilen;
    int size = t->top;
    if (size >= needed) return;
    while (size < needed) size = size * 3 / 2;
    t->data = realloc(t->data, size);
    if (hilen > 0) memmove(&t->data[size - hilen], &t->data[t->hi], hilen);
    t->hi = size - hilen;
    t->top = size;
}

extern inline length lengthText(text *t) {
    return t->lo + (t->top - t->hi);
}

// Move the gap to the given position.
static void moveGap(text *t, point at) {
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

// Index the text as if it had no gap.
static char charAt(text *t, point at) {
    if (at >= lengthText(t)) at = lengthText(t) - 1;
    if (at < t->lo) return t->data[at];
    else return t->data[t->hi + (at - t->lo)];
}

// Clean up insertion text, assumed to be UTF-8 valid. Remove nulls and
// carriage returns and make these adjustments:
//   internal trailing spaces  ...[... \n...]...  ->  ...[...\n...]...
//   final trailing spaces     ...[... ]\n...     ->  ...[...]\n...
//   initial trailing spaces   ... [\n...]...     ->  ??? ...[...]\n... (Del)
//   trailing blank lines      ...[...\n]\n       ->  ...[...]\n
//   trailing blank lines      ...[...\n\n]       ->  ...[...\n]
//   trailing blank lines      ...\n[\n]          ->  ...\n[]
//   missing final newline     ...[...]           ->  ...[...\n] (Separate!)
static chars *clean(text *t, point at, chars s[]) {
    length n = lengthArray(s);
    int j = 0;
    for (int i = 0; i < n; i++) {
        char ch = s[i];
        if (ch == '\0' || ch == '\r') continue;
        if (ch == '\n') {
            while (j > 0 && s[j-1] == ' ') j--;
        }
        s[j++] = ch;
    }
    int max = lengthText(t);
    if (at < max && charAt(t, at) == '\n') {
        while (j > 0 && s[j-1] == ' ') j--;
    }
    if (at == max - 1) {
        while (j > 0 && s[j-1] == '\n') j--;
    }
    if (at == max) {
        while (j > 0 && s[j-1] == '\n') j--;
    }
    if (at == max && j > 0) {
        while (j > 0 && s[j-1] == '\n') j--;
        if (n > 0 && s[j-1] != '\n') s[j++] = '\n';
    }
    setLength(s, j);
    s[j] = '\0';
    return s;
}

// Call this if a change was made near to the end of the text. Add a final
// newline, or remove trailing blank lines. It doesn't matter if this fix is not
// recorded in the undo info, because calling this again after the undo will
// restore the original state.
static void fixEnd(text *t) {
    int n = lengthText(t);
    if (t->lo == t->hi) resizeText(t, 1);
    moveGap(n);
    if (t->data[t->lo - 1] != '\n') t->data[t->lo++] = '\n';
    else while (t->lo >= 2 && t->data[t->lo - 2] == '\n') t->lo--;
}

    // TODO ...[...\n]\n  ->  ...[...]\n

bool loadText(text *t, chars *buffer) {
    bool ok = uvalid(lengthArray(buffer), buffer, true);
    if (! ok) return false;
    length n = lengthArray(buffer);
    t->lo = 0;
    t->hi = t->top;
    resizeText(t, n);
    buffer = clean(t, 0, buffer);
    resizeArray(buffer, n);
    t->hi = t->top - n;
    memcpy(&t->data[t->hi], buffer, n);
    return true;
}

chars *getText(text *t, point at, length n, chars s[]) {
    assert(at + n <= lengthText(t));
    moveGap(t, at + n);
    resizeArray(s, n);
    memcpy(s, &t->data[at], n);
    s[n] = '\0';
    return s;
}

// Get rid of invalid UTF-8 bytes and clean up.
static chars *preInsertText(text *t, point at, chars s[]) {
    length n = lengthArray(s);
    int i = 0, j = 0;
    codePoint cp = getCodePoint();
    while (i < n) {
        nextCode(&cp, &s[i]);
        if (cp.code != UBAD) {
            for (int k = 0; k < cp.length; k++) {
                s[j++] = s[i++];
            }
        }
        else i = i + cp.length;
    }
    setLength(s, j);
    s[j] = '\0';
    return clean(t, at, s);
}

chars *insertText(text *t, point at, chars s[]) {
    s = preInsertText(t, at, s);
    int n = lengthArray(s);
    moveGap(t, at);
    if (n > t->hi - t->lo) resizeText(t, n);
    memcpy(&t->data[t->lo], s, n);
    t->lo = t->lo + n;
    return n;
}

// Delete n bytes after the given position. The request is adjusted as necessary
// to maintain the invariants. The bytes actually deleted are returned in the
// given array, possibly resized.
chars *deleteText(text *t, point at, length n, chars s[]);
// ..._[...]\n   ->  ...[_...]\n
// ..._[...\n]   ->  ...[_...]\n   !!!
// ...[...\n]    ->  ...[...]\n    !!!
// ...\n\n[...]  ->  ...\n[\n...]
// ...\n[...]\n  ->  ...[\n...]\n
void moveText(text *t, point at);

// Is the cursor position OK to delete backwards from? If the position is at the
// end, what remains, if anything, must have at least one newline at the end.
bool validDeleteText(text *t, point at, length n) {
    length len = lengthText(t);
    if (at < len) return true;
    if (n > at) n = at;
    point p = at - n;
    if (p == 0) return true;
    if (charAt(t, p-1) == '\n') return true;
    return false;
}

// Add multiple final newlines or trailing spaces to the deletion.
// Move to the nearest end of the deletion, in case n is very large.
char const *deleteText(text *t, point at, length n) {
    if (! validDeleteText(t, at, n)) return "";
    length len = lengthText(t);
    if (n > at) n = at;
char pic[100];
printf("B at=%d %s\n", at, show(t,pic));
    if (at == len) {
        while (at - n >= 2 && charAt(t, at-n-2) == '\n') n++;
    }
    else if (at == len - 1) {
        while (at - n > 0 && charAt(t, at-n-1) == '\n') n++;
    }
    if (at < len && charAt(t, at) == '\n') {
        while (at - n > 0 && charAt(t, at-n-1) == ' ') n++;
    }
printf("C at=%d %s\n", at, show(t,pic));
    if (t->lo < at + n / 2) {
        moveGap(t, at);
        t->lo = at - n;
    }
    else {
        moveGap(t, at - n);
        t->hi = t->hi + n;
    }
printf("D at=%d %s\n", at, show(t,pic));
    return t->data + t->hi;
}

void moveText(text *t, point at) {
    if (at > lengthText(t)) at = lengthText(t);
    moveGap(t, at);
}

// Unit testing
#ifdef textTest

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

bool testI(char *s, int at, char *t1, char *t2) {
    text *t = newText();
    char sb[strlen(s)+2];
    strcpy(sb, s);
    char t1b[strlen(t1)+2];
    strcpy(t1b, t1);
    char *dots = strstr(t1b,"...");
    insertText(t, 0, strlen(t1b), t1b);
    if (dots != NULL) {
        int p = (dots - t1b);
        moveGap(t, p+3);
        t->lo = t->lo - 3;
    }
    insertText(t, at, strlen(sb), sb);
    bool ok = compare(t, t2);
    freeText(t);
    return ok;
}

bool testD(int at, int n, char *t1, char *t2) {
    text *t = newText();
    char t1b[strlen(t1)+2];
    strcpy(t1b, t1);
    char *dots = strstr(t1b,"...");
    insertText(t, 0, strlen(t1b), t1b);
    if (dots != NULL) {
        int p = (dots - t1b);
        moveGap(t, p+3);
        t->lo = t->lo - 3;
    }
char pic[100];
printf("A %s\n", show(t,pic));
    deleteText(t, at, n);
    bool ok = compare(t, t2);
    freeText(t);
    return ok;
}

int main() {
    setbuf(stdout, NULL);
    assert(testI("abcdm\n",   0, "", "abcdm\n..."));
    assert(testI("abcdm\r\n", 0, "", "abcdm\n..."));
    assert(testI("abcdm  \n", 0, "", "abcdm\n..."));
    assert(testI("abcdm\n\n", 0, "", "abcdm\n..."));
    assert(testI("abcdm",     0, "", "abcdm\n..."));
    assert(testI("efghijkl",  4, "abcdm\n", "abcdefghijkl...m\n"));
    assert(testI("efghijkl",  4, "a...bcdm\n", "abcdefghijkl...m\n"));
    assert(testD(3, 1, "abxcd\n", "ab...cd\n"));
/*
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
    */
    printf("Text module OK\n");
    return 0;
}

#endif
