// The Snipe editor is free and open source, see licence.txt.
#include "cursor.h"
#include "style.h"
#include "line.h"
#include "theme.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// A cursor has a current position in the text 'at'. If there is a selection, it
// runs from 'from' to 'at', otherwise 'from' = 'at'. There is a column position
// 'col' which is normally negative but, when moving the cursor up or down, it
// represents a virtual column beyond the end of the current line, to which the
// cursor returns if moved to a line which is long enough. For 'word' based
// edits, the token boundaries produced by scanning are used.
struct cursor { int at, from, col; };
typedef struct cursor cursor;

// Multiple cursors are held in a flexible array of structures, with access to
// the line boundaries and styles. The styles need to be up to date before
// 'word' based edits. The current cursor corresponding to the most recent Point
// or AddPoint action is tracked to match up with the next Select action.
struct cursors {
    ints *lines;
    chars *styles;
    int length, capacity, current;
    struct cursor *cs;
};

cursors *newCursors(ints *lines, chars *styles) {
    int initialCapacity = 10;
    cursors *cs = malloc(sizeof(cursors));
    *cs = (cursors) {
        .lines = lines, .styles = styles, .current = 0, .length = 1,
        .capacity = initialCapacity
    };
    cs->cs = malloc(initialCapacity * sizeof(cursor));
    cs->cs[0] = (cursor) { .at = 0, .from = 0, .col = 0 };
    return cs;
}

void freeCursors(cursors *cs) {
    free(cs->cs);
    free(cs);
}

int countCursors(cursors *cs) {
    return cs->length;
}

int cursorAt(cursors *cs, int i) {
    return cs->cs[i].at;
}

int cursorFrom(cursors *cs, int i) {
    return cs->cs[i].from;
}

// Increase the number of available cursors.
static void increaseCursors(cursors *cs) {
    cs->capacity += 10;
    cs->cs = realloc(cs->cs, cs->capacity * sizeof(cursor));
}

// Insert a new cursor at index i.
static void addCursor(cursors *cs, int i, int p) {
    if (cs->length >= cs->capacity) increaseCursors(cs);
    memmove(&cs->cs[i+1], &cs->cs[i], (cs->length - i) * sizeof(cursor));
    cs->length++;
    cs->cs[1] = (cursor) { .at = p, .from = p, .col = -1 };
}

// Delete the cursor at index i.
static void deleteCursor(cursors *cs, int i) {
    cs->length--;
    memmove(&cs->cs[i], &cs->cs[i+1], (cs->length - i) * sizeof(cursor));
}

// Adjust the cursor positions following an insertion or deletion.
void updateCursors(cursors *cs, int p, int n) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (c->at >= p) {
            c->at = c->at + n;
            if (c->at < p) c->at = p;
        }
        if (c->from >= p) {
            c->from = c->from + n;
            if (c->from < p) c->from = p;
        }
    }
}

int maxRow(cursors *cs) {
    int pos = 0;
    for (int i = 0; i < cs->length - 1; i++) {
        if (cs->cs[i].at > pos) pos = cs->cs[i].at;
        if (cs->cs[i].from > pos) pos = cs->cs[i].from;
    }
    return findRow(cs->lines, pos);
}

// Check if there is a current selection.
static inline bool selecting(cursor *c) { return c->from != c->at; }

// Collapse the selection forwards.
static inline void collapseF(cursor *c) { c->from = c->at; }

// Collapse the selection backwards.
static inline void collapseB(cursor *c) { c->at = c->from; }

// Collapse the selection leftwards.
static inline void collapseL(cursor *c) {
    if (c->from > c->at) c->from = c->at;
    else c->at = c->from;
}

// Collapse the selection rightwards.
static inline void collapseR(cursor *c) {
    if (c->from < c->at) c->from = c->at;
    else c->at = c->from;
}

// Check for overlapping cursors and merge them.
void mergeCursors(cursors *cs) {
    for (int i = 0; i < cs->length - 1; i++) {
        cursor *c = &cs->cs[i];
        cursor *d = &cs->cs[i+1];
        if (d->at <= c->at || d->from < c->from ||
            d->from < c->at || d->at < c->from
        ) {
            deleteCursor(cs, i+1);
        }
    }
}

// Mark left, including when selecting or not
static void ifMarkLeftChar(cursors *cs, bool ifSelect) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (! ifSelect && selecting(c)) continue;
        if (c->at > 0) c->at--;
        c->col = -1;
    }
    mergeCursors(cs);
}

void markLeftChar(cursors *cs) { ifMarkLeftChar(cs, true); }

void pMarkLeftChar(cursors *cs) { ifMarkLeftChar(cs, false); }

// Mark left, including when selecting or not
static void ifMarkRightChar(cursors *cs, bool ifSelect) {
    int end = startLine(cs->lines, length(cs->lines));
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (! ifSelect && selecting(c)) continue;
        if (c->at < end) c->at++;
        c->col = -1;
    }
    mergeCursors(cs);
}

void markRightChar(cursors *cs) { ifMarkRightChar(cs, true); }

void pMarkRightChar(cursors *cs) { ifMarkRightChar(cs, false); }

void markLeftWord(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (c->at > 0 && hasStyleFlag(C(cs->styles)[c->at], START)) c->at--;
        while (! hasStyleFlag(C(cs->styles)[c->at], START)) c->at--;
        c->col = -1;
    }
    mergeCursors(cs);
}

void markRightWord(cursors *cs) {
    int end = startLine(cs->lines, length(cs->lines));
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (c->at < end && hasStyleFlag(C(cs->styles)[c->at], START)) c->at++;
        while (! hasStyleFlag(C(cs->styles)[c->at], START)) c->at++;
        c->col = -1;
    }
    mergeCursors(cs);
}

void markUpLine(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        int row = findRow(cs->lines, c->at);
        if (row == 0) return;
        int start = startLine(cs->lines, row);
        int prev = startLine(cs->lines, row - 1);
        if (c->col < 0) c->col = c->at - start;
        c->at = prev + c->col;
        if (c->at >= start) c->at = start - 1;
    }
    mergeCursors(cs);
}

void markDownLine(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        int row = findRow(cs->lines, c->at);
        if (row == length(cs->lines)) return;
        int start = startLine(cs->lines, row);
        int next = startLine(cs->lines, row + 1);
        int nextEnd = endLine(cs->lines, row + 1);
        if (c->col < 0) c->col = c->at - start;
        c->at = next + c->col;
        if (c->at >= nextEnd) c->at = nextEnd - 1;
    }
    mergeCursors(cs);
}

void markStartLine(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        int row = findRow(cs->lines, c->at);
        int start = startLine(cs->lines, row);
        c->at = start;
        c->col = -1;
    }
    mergeCursors(cs);
}

void markEndLine(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        int row = findRow(cs->lines, c->at);
        if (row == length(cs->lines)) return;
        int end = endLine(cs->lines, row);
        c->at = end - 1;
        c->col = -1;
    }
    mergeCursors(cs);
}

void moveLeftChar(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (selecting(c)) collapseL(c);
        else {
            if (c->at > 0) c->at--;
            c->from = c->at;
            c->col = -1;
        }
    }
    mergeCursors(cs);
}

void moveRightChar(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (selecting(c)) collapseR(c);
        else {
            int end = startLine(cs->lines, length(cs->lines));
            if (c->at < end) c->at++;
            c->from = c->at;
            c->col = -1;
        }
    }
    mergeCursors(cs);
}

void moveLeftWord(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (selecting(c)) collapseL(c);
        else {
            if (c->at > 0 && hasStyleFlag(C(cs->styles)[c->at], START)) c->at--;
            while (! hasStyleFlag(C(cs->styles)[c->at], START)) c->at--;
            c->col = -1;
            c->from = c->at;
        }
    }
    mergeCursors(cs);
}

void moveRightWord(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (selecting(c)) collapseR(c);
        else {
            int end = startLine(cs->lines, length(cs->lines));
            if (c->at < end && hasStyleFlag(C(cs->styles)[c->at], START)) c->at++;
            while (! hasStyleFlag(C(cs->styles)[c->at], START)) c->at++;
            c->col = -1;
            c->from = c->at;
        }
    }
    mergeCursors(cs);
}

void moveUpLine(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (selecting(c)) collapseL(c);
        else {
            int row = findRow(cs->lines, c->at);
            if (row == 0) continue;
            int start = startLine(cs->lines, row);
            int prev = startLine(cs->lines, row - 1);
            if (c->col < 0) c->col = c->at - start;
            c->at = prev + c->col;
            if (c->at >= start) c->at = start - 1;
            c->from = c->at;
        }
    }
    mergeCursors(cs);
}

void moveDownLine(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (selecting(c)) collapseR(c);
        else {
            int row = findRow(cs->lines, c->at);
            if (row == length(cs->lines)) continue;
            int start = startLine(cs->lines, row);
            int next = startLine(cs->lines, row + 1);
            int nextEnd = endLine(cs->lines, row + 1);
            if (c->col < 0) c->col = c->at - start;
            c->at = next + c->col;
            if (c->at >= nextEnd) c->at = nextEnd - 1;
            c->from = c->at;
        }
    }
    mergeCursors(cs);
}

void moveStartLine(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (selecting(c)) collapseL(c);
        else {
            int row = findRow(cs->lines, c->at);
            int start = startLine(cs->lines, row);
            c->at = start;
            c->from = c->at;
            c->col = -1;
        }
    }
    mergeCursors(cs);
}

void moveEndLine(cursors *cs) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (selecting(c)) collapseR(c);
        else markEndLine(cs);
        c->from = c->at;
    }
    mergeCursors(cs);
}

void point(cursors *cs, int p) {
    cs->length = 1;
    cs->current = 0;
    cursor *c = &cs->cs[0];
    c->at = c->from = p;
    c->col = -1;
}

// If the click is at the same point as an old cursor, delete it. Keep the
// cursors in order of text position, making sure the main cursor remains first.
void addPoint(cursors *cs, int p) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (c->at < p && i < cs->length - 1) continue;
        if (c->at < p) {
            addCursor(cs, i + 1, p);
            cs->current = i + 1;
        }
        else if (c->at == p) {
            if (cs->length == 1) {
                cs->current = 0;
            }
            else if (i > 0){
                deleteCursor(cs, i);
                cs->current = i - 1;
            }
            else {
                deleteCursor(cs, i);
                cs->current = i;
            }
        }
        else {
            addCursor(cs, i, p);
            cs->current = i;
        }
    }
}

// Can follow Point or AddPoint.
void doSelect(cursors *cs, int p) {
    cursor *c = &cs->cs[cs->current];
    c->from = c->at;
    c->at = p;
    c->col = -1;
}


void applyCursors(cursors *cs, int row, chars *styles) {
    int n = length(styles);
    int start = startLine(cs->lines, row);
    int end = start + n;
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->cs[i];
        if (c->at < start && c->from <= start) continue;
        if (c->at > end && c->from >= end) continue;
        if (c->at >= start && c->at < end) {
            char st = addStyleFlag(C(styles)[c->at - start], POINT);
            C(styles)[c->at - start] = st;
        }
        for (int i = 0; i < n; i++) {
            if ((c->at <= start + i && start + i + 1 <= c->from) ||
                (c->from <= start + i && start + i + 1 <= c->at)) {
                char st = addStyleFlag(C(styles)[i], SELECT);
                C(styles)[i] = st;
            }
        }
    }
}

#ifdef test_cursor

// During testing, a text pattern has ; . [] ][ to mean newline, cursor
// position, and forward or backward selection.

// Create cursor i from a text pattern.
static void makeCursor(cursors *cs, int i, char *pattern) {
    cursor *c = &cs->cs[i];
    c->at = -1; c->from = -1; c->col = -1;
    for (int i = 0; i < strlen(pattern); i++) {
        if (pattern[i] == '.') {
            c->at = c->from = i;
            return;
        }
        else if (pattern[i] == '[') {
            if (c->from < 0) c->from = c->at = i;
            else c->from = i - 1;
        }
        else if (pattern[i] == ']') {
            if (c->at < 0) c->at = c->from = i;
            else c->at = i - 1;
        }
    }
}

// Make a list of lines from a text pattern.
static ints *makeLines(char *pattern) {
    ints *lines = newInts();
    int p = 0;
    for (int i = 0; i < strlen(pattern); i++) {
        char ch = pattern[i];
        if (ch == '.' || ch == '[' || ch == ']') continue;
        if (ch == ';') {
            int n = length(lines);
            resize(lines, n + 1);
            I(lines)[n] = p + 1;
        }
        p++;
    }
    return lines;
}

// The type of a move/mark cursor function.
typedef void moveMark(cursors *cs);

// Check a function with one cursor
static bool check(moveMark f, cursors *cs, char *after) {
    f(cs);
    makeCursor(cs, 1, after);
    cursor *c = &cs->cs[0];
    cursor *ac = &cs->cs[1];
    if (ac->at != c->at) printf("at %d %d\n", c->at, ac->at);
    if (ac->from != c->from) printf("fr %d %d\n", c->from, ac->from);
    bool ok = ac->at == c->at && ac->from == c->from;
    return ok;
}

static void testLeftRight() {
    ints *lines = makeLines(".ab;c;");
    chars *styles = NULL;
    cursors *cs = newCursors(lines, styles);
    makeCursor(cs, 0, ".ab;c;");
    assert(check(moveRightChar, cs, "a.b;c;"));
    assert(check(moveRightChar, cs, "ab.;c;"));
    assert(check(moveRightChar, cs, "ab;.c;"));
    assert(check(moveRightChar, cs, "ab;c.;"));
    assert(check(moveRightChar, cs, "ab;c;."));
    assert(check(moveRightChar, cs, "ab;c;."));
    assert(check(moveLeftChar, cs, "ab;c.;"));
    assert(check(moveLeftChar, cs, "ab;.c;"));
    assert(check(moveLeftChar, cs, "ab.;c;"));
    assert(check(moveLeftChar, cs, "a.b;c;"));
    assert(check(moveLeftChar, cs, ".ab;c;"));
    assert(check(moveLeftChar, cs, ".ab;c;"));
    assert(check(moveEndLine, cs, "ab.;c;"));
    assert(check(moveStartLine, cs, ".ab;c;"));
    makeCursor(cs, 0, "[ab];c;");
    assert(check(moveRightChar, cs, "ab.;c;"));
    makeCursor(cs, 0, "]ab[;c;");
    assert(check(moveRightChar, cs, "ab.;c;"));
    freeCursors(cs);
    freeList(lines);
}

static void testUpDown() {
    ints *lines = makeLines("ab.c;d;efg;");
    chars *styles = NULL;
    cursors *cs = newCursors(lines, styles);
    makeCursor(cs, 0, "ab.c;d;efg;");
    assert(check(moveDownLine, cs, "abc;d.;efg;"));
    assert(check(moveDownLine, cs, "abc;d;ef.g;"));
    assert(check(moveDownLine, cs, "abc;d;efg;."));
    assert(check(moveDownLine, cs, "abc;d;efg;."));
    assert(check(moveUpLine, cs, "abc;d;ef.g;"));
    assert(check(moveUpLine, cs, "abc;d.;efg;"));
    assert(check(moveUpLine, cs, "ab.c;d;efg;"));
    assert(check(moveUpLine, cs, "ab.c;d;efg;"));
    freeCursors(cs);
    freeList(lines);
}

static void testMoveWord() {
    ints *lines = makeLines(".int n=42;x;");
    style K = addStyleFlag(KEY, START), k = KEY;
    style G = addStyleFlag(GAP, START);
    style I = addStyleFlag(ID, START);
    style S = addStyleFlag(SIGN, START);
    style N = addStyleFlag(NUMBER, START), n = NUMBER;
    char st[] = { K,k,k,G,I,S,N,n,G,I,G };
    chars *styles = newChars();
    resize(styles, 11);
    for (int i=0; i<11; i++) C(styles)[i] = st[i];
//    insert(styles, 0, 11, st);
    cursors *cs = newCursors(lines, styles);
    makeCursor(cs, 0, ".int n=42;x;");
    assert(check(moveRightWord, cs, "int. n=42;x;"));
    assert(check(moveRightWord, cs, "int .n=42;x;"));
    assert(check(moveRightWord, cs, "int n.=42;x;"));
    assert(check(moveRightWord, cs, "int n=.42;x;"));
    assert(check(moveRightWord, cs, "int n=42.;x;"));
    assert(check(moveRightWord, cs, "int n=42;.x;"));
    assert(check(moveLeftWord, cs, "int n=42.;x;"));
    assert(check(moveLeftWord, cs, "int n=.42;x;"));
    assert(check(moveLeftWord, cs, "int n.=42;x;"));
    assert(check(moveLeftWord, cs, "int .n=42;x;"));
    assert(check(moveLeftWord, cs, "int. n=42;x;"));
    assert(check(moveLeftWord, cs, ".int n=42;x;"));
    freeCursors(cs);
    freeList(lines);
    freeList(styles);
}

static void testMark() {
    ints *lines = makeLines("a.b;c;");
    chars *styles = NULL;
    cursors *cs = newCursors(lines, styles);
    makeCursor(cs, 0, "a.b;c;");
    assert(check(markRightChar, cs, "a[b];c;"));
    assert(check(markRightChar, cs, "a[b;]c;"));
    assert(check(markRightChar, cs, "a[b;c];"));
    assert(check(markLeftChar, cs, "a[b;]c;"));
    assert(check(markLeftChar, cs, "a[b];c;"));
    assert(check(markLeftChar, cs, "a.b;c;"));
    assert(check(moveDownLine, cs, "ab;c.;"));
    assert(check(markLeftChar, cs, "ab;]c[;"));
    assert(check(markLeftChar, cs, "ab];c[;"));
    assert(check(markLeftChar, cs, "a]b;c[;"));
    assert(check(markRightChar, cs, "ab];c[;"));
    assert(check(markRightChar, cs, "ab;]c[;"));
    assert(check(markRightChar, cs, "ab;c.;"));
    assert(check(moveRightChar, cs, "ab;c;."));
    assert(check(markUpLine, cs, "ab;]c;["));
    assert(check(markUpLine, cs, "]ab;c;["));
    assert(check(moveLeftChar, cs, ".ab;c;"));
    assert(check(markDownLine, cs, "[ab;]c;"));
    assert(check(markDownLine, cs, "[ab;c;]"));
    assert(check(moveLeftChar, cs, ".ab;c;"));
    freeCursors(cs);
    freeList(lines);
}

static void testMarkWord() {
    ints *lines = makeLines(".int n=42;x;");
    style K = addStyleFlag(KEY, START), k = KEY;
    style G = addStyleFlag(GAP, START);
    style I = addStyleFlag(ID, START);
    style S = addStyleFlag(SIGN, START);
    style N = addStyleFlag(NUMBER, START), n = NUMBER;
    char st[] = { K,k,k,G,I,S,N,n,G,I,G };
    chars *styles = newChars();
    resize(styles, 11);
    for (int i=0; i<11; i++) C(styles)[i] = st[i];
//    insert(styles, 0, 11, st);
    cursors *cs = newCursors(lines, styles);
    makeCursor(cs, 0, ".int n=42;x;");
    assert(check(markRightWord, cs, "[int] n=42;x;"));
    assert(check(markRightWord, cs, "[int ]n=42;x;"));
    assert(check(markRightWord, cs, "[int n]=42;x;"));
    assert(check(markRightWord, cs, "[int n=]42;x;"));
    assert(check(markRightWord, cs, "[int n=42];x;"));
    assert(check(markRightWord, cs, "[int n=42;]x;"));
    assert(check(markLeftWord, cs, "[int n=42];x;"));
    assert(check(markLeftWord, cs, "[int n=]42;x;"));
    assert(check(markLeftWord, cs, "[int n]=42;x;"));
    assert(check(markLeftWord, cs, "[int ]n=42;x;"));
    assert(check(markLeftWord, cs, "[int] n=42;x;"));
    assert(check(markLeftWord, cs, ".int n=42;x;"));
    freeCursors(cs);
    freeList(lines);
    freeList(styles);
}

int main() {
    testLeftRight();
    testUpDown();
    testMoveWord();
    testMark();
    testMarkWord();
    printf("Cursor module OK\n");
    return 0;
}

#endif
