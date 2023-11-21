// Snipe text handling. Free and open source, see licence.txt.
#include "text.h"
#include "unicode.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// TODO: try: gap is at cursor position (row, col).
// There are trailers to make col OK, and enders to make row OK.
// Save is not too hard.
// Insert is rightward at cursor position.
// Delete is leftward at cursor position.
// (Use r,c to get left point of deletion and therefore n ?)
// (What about getting current row for display? Two parts?)
// Pass target row to move(), to avoid recalc.

// A text object stores an array of bytes, between indexes 0 and 'end' of the
// data array, with a gap between 'lo' and 'hi' at the current (row,col) cursor
// position. The current row is equal to the number of newlines before the gap.
// The text object also tracks the number of rows, equal to the number of
// newlines. Line boundaries are found by searching for newlines, either side of
// the gap.
struct text { int rows, row, lo, hi, end; char *data; };

text *newText() {
    int n = 24;
    text *t = malloc(sizeof(text));
    char *data = malloc(n);
    *t = (text) { .rows=0, .row=0, .lo=0, .hi=n, .end=n, .data=data };
    return t;
}

void freeText(text *t) {
    free(t->data);
    free(t);
}

// Resize to make room for an insertion of n bytes.
static void resize(text *t, int n) {
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

// Move the gap to the given index (not in the gap), and update the current row.
static void move(text *t, int to) {
    if (to < t->lo) {
        for (int i = to; i < t->lo; i++) if (t->data[i] == '\n') t->row--;
        int len = (t->lo - to);
        memmove(&t->data[t->hi - len], &t->data[to], len);
        t->hi = t->hi - len;
        t->lo = t->lo - len;
    }
    else {
        for (int i = t->hi; i < to; i++) if (t->data[i] == '\n') t->row++;
        int len = (to - t->hi);
        memmove(&t->data[t->lo], &t->data[t->hi], len);
        t->hi = t->hi + len;
        t->lo = t->lo + len;
    }
}
//------------------------------------------------------------------------------
// From now on, ...

// Find the index of the given row (< t->row).
static int findLeft(text *t, int row) {
    int i = t->lo;
    for (int j = 0; j <= t->row - row; j++) {
        if (j > 0) i--;
        while (i > 0 && t->data[i-1] != '\n') i--;
    }
    return i;
}

// Find the index of the given row (>= t->row).
static int findRight(text *t, int row) {
    int i = t->hi;
    for (int j = 0; j < row - t->row; j++) {
        while (t->data[i] != '\n') i++;
        i++;
    }
    return i;
}

// Point to the given line, assuming that the gap is on a line boundary (i.e.
// just after a newline) so that the line doesn't straddle the gap.
char const *line(text *t, int row) {
    if (row >= t->rows) return "\n";
    else if (row < t->row) return &t->data[findLeft(t, row)];
    else return &t->data[findRight(t, row)];
}

// Prepare for insert/delete; add final newlines to make the row valid.
static void addLines(text *t, int row) {
    move(t, t->rows);
    while (row > t->rows - 1) {
        t->data[--t->hi] = '\n';
        t->rows++;
    }
}

// Prepare for insert/delete; add n spaces to the line before the gap.
static void addSpaces(text *t, int n) {
    int end = t->lo - 1;
    for (int i = end; i < end + n; i++) t->data[i] = ' ';
    t->data[end + n] = '\n';
    t->lo = t->lo + n;
}

// Remove trailing spaces from the line immediately before the gap.
static void repairSpaces(text *t) {
    while (t->lo >= 2 && t->data[t->lo-2] == ' ') {
        t->data[t->lo - 2] = '\n';
        t->lo--;
    }
}

// Make sure there is a final newline, and remove any trailing blank lines.
static void repairLines(text *t) {
    move(t, t->rows);
    if (t->lo > 0 && t->data[t->lo-1] != '\n') {
        resize(t, 1);
        t->data[t->lo++] = '\n';
    }
    while (t->lo >= 2 &&
        t->data[t->lo-1] == '\n' &&
        t->data[t->lo-2] == '\n'
    ) {
        t->lo--;
        t->rows--;
        t->row--;
    }
}
//----------
// If the insertion is not utf-8 valid, return false. Make room for inserting n
// bytes, plus col to cover any trailing spaces needed, plus row to cover any
// final newlines needed. Put the gap after the relevant line and add any
// necessary trailers. Move the insert into place. Scan the insert, count the
// newlines, and convert other control characters to spaces. Repair trailers,
// and repair final newlines if appropriate.
bool insert(text *t, point p, int n, char s[n]) {
    if (! uvalid(n, s)) return false;
    resize(t, n + p.col + p.row);
    if (p.row >= t->rows) addLines(t, p.row);




    move(t, p.row+1);
    int in = findLeft(t, p.row) + p.col;
    if (in >= t->lo) addSpaces(t, in - t->lo + 1);
    memmove(&t->data[in+n], &t->data[in], t->lo-in);
    memmove(&t->data[in], s, n);
    t->lo += n;
    int newlines = 0;
    for (int i = in; i < in+n; i++) {
        if (t->data[i] == '\n') newlines++;
        else if (t->data[i] < ' ') t->data[i] = ' ';
    }
    t->rows += newlines;
    t->row += newlines;
    for (int row = t->row - newlines; row <= t->row; row++) {
        move(t, row);
        repairTrailers(t);
    }
    if (t->end - t->hi < 2) repairEnders(t);
    return true;
}

int measure(text *t, point p, point q) {
    return 0;
}

void delete(text *t, point p, point q, char s[]) {

}

void oldelete(text *t, point p, int n, char s[n]) {
    if (p.row >= t->rows) addLines(t, p.row);
    move(t, p.row);
    int end = findRight(t, p.row+1);
    int out = t->hi + p.col;
}

/*
// Insert a string into a line, assuming the string is clean and contains no
// newlines. Make room for the insert, plus any newlines or spaces needed. Add
// final newlines if necessary. Add trailing spaces to the line if necessary.
// Do the insertion, then repair.
bool insert1(text *t, point p, int n, char s[n]) {
    if (gap(t) < p.row + p.col + n) resize(t, p.row + p.col + n);
    if (p.row >= t->rows) addLines(t, p.row);
    move(t, p.row+1);
    int start = findLeft(t, p.row) + p.col;
    if (start >= t->lo) addSpaces(t, start - t->lo + 1);
    memmove(&t->data[start+n], &t->data[start], t->lo-start);
    memmove(&t->data[start], s, n);
    t->lo += n;
    repair(t);
    return true; // check uvalid, no nl
}

// Split at the given row/col, i.e. insert a newline.
void split(text *t, point p) {
    if (gap(t) < p.row + p.col + 1) resize(t, p.row + p.col + 1);
    if (p.row >= t->rows) addLines(t, p.row);
    move(t, p.row+1);
    int start = findLeft(t, p.row) + p.col;
    if (start >= t->lo) addSpaces(t, start - t->lo + 1);
    int len = t->lo - start;
    memmove(&t->data[t->hi - len], &t->data[start], len);
    t->hi = t->hi - len;
    t->lo = t->lo - len;
    t->data[t->lo++] = '\n';
    t->rows++;
    repair(t);
}
*/
// -----------------------------------------------------------------------------

int rows(text *t) {
    return t->rows;
}


// resize, addLines.
// find start and put gap at end.
// jiggle, repair

// Move the gap to the given column in the current row (row < rows). Insert
// trailing spaces if necessary.
// static void moveCol(text *t, int col) {
//     int len = find(t, row + 1) - t->hi - 1;
//     if (col <= len) moveIndex(t, t->lo + p.col);
//     else {
//         moveIndex(t, t->lo + len);
//         for (int i = 0; i < col - len; i++) t->data[t->lo++] = ' ';
//     }
// }
//
// // Clean up the given insert string, replacing control characters by spaces, and
// // returning false if not utf8-valid.
// static bool clean(int n, char s[n]) {
//     bool high = false;
//     for (int i = 0; i < n; i++) {
//         if ((s[i] & 0x80) != 0) high = true;
//         else if (s[i] < ' ' && s[i] != '\n') s[i] = ' ';
//     }
//     if (high) return uvalid(n, s);
//     else return true;
// }
//
// // Repair everything from start (the old gap point) to the current gap
// // point, and leave the gap at a row boundary.
// static void repair(text *t, int start) {
//     for (int i = start; i < t->lo; i++) {
//         if (t->data[i] ==);
//     }
// }
//
// //------------------------------------------------------
//
// // Clean up the insertion string. Make room for the insert, plus any newlines or
// // spaces needed. Add final newlines if necessary. Add trailing spaces to the
// // line if necessary. Move the gap to the given point. Insert into the gap.
// // Move the gap back to a line boundary. Update the row count. Remove any
// // trailing spaces or final newlines.
// bool insert(text *t, point p, int n, char s[n]) {
//     if (! clean(n, s)) return false;
//     if (gap(t) < p.row + p.col + n) resize(t, p.row + p.col + n);
//     if (p.row >= t->rows) addLines(t, p.row);
//     moveRow(t, p.row);
//     int oldStart = t->lo;
//     moveCol(t, p.col);
//     memmove(&t->data[t->lo], s, n);
//     t->lo += n;
//
//
//     for (int i = 0; i < n; i++) if (s[i] == '\n') t->rows++;
//     // repair.
//     moveRow(t, p.row - 1);
//     return true;
// }

// edit *newEdit(int n, edit *old) {
//     if (old != NULL && old->max > n) return old;
//     edit *e = realloc(old, sizeof(edit) + n + 1);
//     e->at = e->to = e->n = 0;
//     e->max = n;
//     return e;
// }
//
// extern inline int lengthText(text *t) {
//     return t->lo + (t->end - t->hi);
// }
//
// // Clean up new text, assumed to be UTF-8 valid. Normalise line endings, and
// // remove internal trailing spaces.
// static int clean(int n, char *s) {
//     int j = 0;
//     for (int i = 0; i < n; i++) {
//         int ch = s[i];
//         if (ch == 0xE2 && (int)s[i+1] == 0x80 && (int)s[i+2] == 0xA8) {
//             ch = '\n';
//             i = i + 2;
//         }
//         else if (ch == 0xE2 && (int)s[i+1] == 0x80 && (int)s[i+2] == 0xA9) {
//             ch = '\n';
//             i = i + 2;
//         }
//         else if (ch == '\r' && s[i+1] != '\n') {
//             ch = '\n';
//             i = i + 1;
//         }
//         else if (ch == '\r') continue;
//         else if (ch == '\n') {
//             while (j > 0 && s[j-1] == ' ') j--;
//         }
//         s[j++] = ch;
//     }
//     n = j;
//     s[n] = '\0';
//     return n;
// }
//
// // Clean the buffer, and also remove any final trailing spaces or blank lines,
// // and ensure a final newline. Then copy the buffer as the new text.
// bool loadText(text *t, int n, char *buffer) {
//     bool ok = uvalid(n, buffer);
//     if (! ok) return false;
//     n = clean(n, buffer);
//     while (n > 0 && buffer[n-1] == ' ') n--;
//     if (n > 0 && buffer[n-1] != '\n') buffer[n++] = '\n';
//     while (n > 1 && buffer[n-2] == '\n') n--;
//     t->lo = 0;
//     t->hi = t->end;
//     resizeText(t, n);
//     t->hi = t->end - n;
//     memcpy(&t->data[t->hi], buffer, n);
//     return true;
// }
//
// // Insert text. As well as cleaning the text, adjust it to avoid creating
// // trailing spaces, blank lines or a missing final newline in context.
// edit *insertText(text *t, edit *e) {
//     int len = lengthText(t);
//     assert(0 <= e->at && e->at <= e->to && e->to <= len && e->n >= 0);
//     moveIndex(t, e->at);
//     int n = clean(e->n, e->s);
//     if (e->at == len) {
//         while (n > 0 && e->s[n-1] == ' ') n--;
//         if (n > 0 && e->s[n-1] != '\n') e->s[n++] = '\n';
//         while (n > 1 && e->s[n-2] == '\n') n--;
//         if (n == 1 && e->s[0] == '\n') n--;
//     }
//     else if (e->at == len - 1) {
//         while (n > 0 && e->s[n-1] == ' ') n--;
//         while (n > 0 && e->s[n-1] == '\n') n--;
//     }
//     else if (t->data[t->hi] == '\n') {
//         while (n > 0 && e->s[n-1] == ' ') n--;
//     }
//     e->n = n;
//     e->to = e->at;
//     if (n > 0 && e->s[0] == '\n') {
//         while (e->at > 0 && t->data[e->at - 1] == ' ') e->at--;
//     }
//     t->lo = e->at;
//     if (n > t->hi - t->lo) resizeText(t, n);
//     memcpy(&t->data[t->lo], e->s, n);
//     t->lo = t->lo + n;
//     return e;
// }
//
// // Delete a edit. Clean it, and adjust, possibly at both ends, to avoid
// // creating trailing spaces, blank lines or a missing final newline in context.
// edit *deleteText(text *t, edit *e) {
//     int len = lengthText(t);
//     assert(0 <= e->to && e->to < e->at && e->at <= len);
//     moveIndex(t, e->at);
//     if (e->at > 0 && e->at == len) {
//         e->at--;
//         moveIndex(t, e->at);
//     }
//     if (e->at == len - 1) {
//         while (e->to > 0 && t->data[e->to - 1] == '\n') e->to--;
//     }
//     if (t->data[t->hi] == '\n') {
//         while (e->to > 0 && t->data[e->to - 1] == ' ') e->to--;
//     }
//     int n = e->at - e->to;
//     t->lo = t->lo - n;
//     if (n > e->max) e = newEdit(n, e);
//     memcpy(e->s, &t->data[t->lo], n);
//     return e;
// }
//
// edit *getText(text *t, edit *e) {
//     int n = e->to - e->at;
//     if (n > e->max) e = newEdit(n, e);
//     moveIndex(t, e->at);
//     memcpy(e->s, &t->data[e->at], n);
//     e->s[n] = '\0';
//     return e;
// }

#ifdef TESTtext

// Create a text object for testing, from a string with N for newline and dots
// for the gap.
static text *build(char *s) {
    text *t = newText();
    int n = strlen(s);
    t->data = realloc(t->data, n+1);
    t->hi = t->end = n;
    strcpy(t->data, s);
    t->lo = strchr(s, '.') - s;
    t->hi = strrchr(s, '.') - s + 1;
    for (int i = 0; i < strlen(s); i++) {
        if (s[i] == 'N') { t->rows++; t->data[i] = '\n'; }
        else if (s[i] == '.') t->row = t->rows;
    }
    return t;
}

// Visualise a text object for testing. Show newlines as N and the gap as dots.
static char *show(text *t) {
    static char s[100];
    int j = 0;
    for (int i = 0; i < t->end; i++) {
        if (i < t->lo || i >= t->hi) s[j++] = t->data[i];
        else s[j++] = '.';
        if (s[j-1] == '\n') s[j-1] = 'N';
    }
    s[j] = '\0';
    return s;
}

// Compare a text object with a string for testing, where the string has dots
// for the gap and N in place of newline. Also check 'row' and 'rows'.
static bool eq(text *t, char *s) {
    int row = 0, rows = 0;
    for (int i = 0; i < strlen(s); i++) {
        if (s[i] == 'N') rows++;
        else if (s[i] == '.') row = rows;
    }
    char *ts = show(t);
    bool ok = strcmp(ts, s) == 0 && t->row == row && t->rows == rows;
    if (! ok) printf("t = %s %d %d\n", ts, t->row, row);
    return ok;
}

static void testBuildShow() {
    text *t = build("abcdxyzefgN........hijkN");
    assert(eq(t,"abcdxyzefgN........hijkN"));
    freeText(t);
}

static void testMoveResize() {
    text *t = newText();
    assert(eq(t,"........................"));
    strcpy(t->data, "abcd\nefg\n"); t->lo = 9; t->rows = 2; t->row = 2;
    assert(eq(t,"abcdNefgN..............."));
    move(t, 1);
    assert(eq(t,"abcdN...............efgN"));
    move(t, 2);
    assert(eq(t,"abcdNefgN..............."));
    move(t, 0);
    assert(eq(t,"...............abcdNefgN"));
    move(t, 1);
    assert(eq(t,"abcdN...............efgN"));
    resize(t, 15);
    assert(eq(t,"abcdN...............efgN"));
    resize(t, 16);
    assert(eq(t,"abcdN...........................efgN"));
    freeText(t);
}

static void testInsert() {
    text *t = newText();
    insert(t, (point){0,0}, 4, "abcd");
    assert(eq(t,"abcdN..................."));
    insert(t, (point){0,4}, 3, "efg");
    assert(eq(t,"abcdefgN................"));
    insert(t, (point){1,0}, 4, "hijk");
    assert(eq(t,"abcdefgNhijkN..........."));
    insert(t, (point){0,4}, 3, "xyz");
    assert(eq(t,"abcdxyzefgN........hijkN"));
    insert(t, (point){0,10}, 3, "   ");
    assert(eq(t,"abcdxyzefgN....................hijkN"));
    insert(t, (point){1,4}, 3, "   ");
    assert(eq(t,"abcdxyzefgNhijkN...................."));
    insert(t, (point){3,3}, 3, "   ");
    assert(eq(t,"abcdxyzefgNhijkN...................."));
    insert(t, (point){0,4}, 1, "\n");
    assert(eq(t,"abcdN...................xyzefgNhijkN"));
    insert(t, (point){1,3}, 1, "\n");
    assert(eq(t,"abcdNxyzN..................efgNhijkN"));
    insert(t, (point){4,0}, 1, "\n");
    assert(eq(t,"abcdNxyzNefgNhijkN.................."));
    freeText(t);
}

// // Compare text object against pattern.
// static bool compare(text *t, char *p) {
//     char actual[100];
//     show(t, actual);
// //    printf("a=%s\n", actual);
//     return strcmp(actual, p) == 0;
// }
//
// // Test an insertion and its adjustments. The 'before' pattern contains
// // [...] to indicate the inserted text.
// static bool testInsert(char *before, char *after) {
//     char temp[100];
//     strcpy(temp, before);
//     int open = strchr(temp, '[') - temp;
//     int close = strchr(temp, ']') - temp;
//     int len = strlen(temp);
//     text *t = newText();
//     memcpy(t->data, temp, open);
//     memcpy(&t->data[open], &temp[close+1], len - close - 1);
//     t->lo = open + len - close - 1;
//     edit *e = newEdit(100, NULL);
//     e->at = e->to = open;
//     e->n = close - 1 - open;
//     memcpy(e->s, temp + open + 1, e->n);
//     e = insertText(t, e);
//     bool ok = compare(t, after);
//     free(e);
//     freeText(t);
//     return ok;
// }
//
// // Test a deletion and its adjustments. The 'before' pattern contains
// // [...] to indicate the deleted text.
// static bool testDelete(char *before, char *after) {
//     char temp[100];
//     strcpy(temp, before);
//     int open = strchr(temp, '[') - temp;
//     int close = strchr(temp, ']') - temp;
//     int len = strlen(temp);
//     text *t = newText();
//     memcpy(t->data, temp, open);
//     memcpy(&t->data[open], &temp[open+1], close-open-1);
//     memcpy(&t->data[close-1], &temp[close+1], len-close-1);
//     t->lo = len - 2;
//     edit *e = newEdit(100, NULL);
//     e->to = open;
//     e->at = close - 1;
//     e = deleteText(t, e);
//     bool ok = compare(t, after);
//     free(e);
//     freeText(t);
//     return ok;
// }

// Test all the ways in which trailing spaces, trailing blank lines or missing
// final newlines can occur through an insertion or deletion.
int main() {
    testBuildShow();
    testMoveResize();
    testInsert();
    // assert(testInsert("x[\ny\nz\n]\n", "x\ny\nz\n")); // final blanks
    // assert(testInsert("x[\ny\nz\n\n]", "x\ny\nz\n")); // final blanks
    // assert(testInsert("x\ny\nz\n[\n]", "x\ny\nz\n")); // final blanks
    // assert(testInsert("x  [\ny\n]z\n", "x\ny\nz\n")); // trailers at start
    //
    // assert(testDelete("abc[def]ghi\n", "abcghi\n"));
    // assert(testDelete("x\n  [y]\nz\n", "x\n\nz\n"));  // trailers at start
    // assert(testDelete("x\ny\n[z]\n", "x\ny\n"));      // final blanks
    // assert(testDelete("x\ny[\nz\n]", "x\ny\n"));      // final newline

    printf("Text module OK\n");
    return 0;
}

#endif
