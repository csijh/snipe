#include "edit.h"
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct edit { int op, at, to, length, max; char *s; };

edit *newEdit() {
    edit *e = malloc(sizeof(edit));
    *e = (edit) { .op = -1, .length = 0, .max = 23, .s = malloc(24) };
    e->s[0] = '\0';
    return e;
}

void freeEdit(edit *e) {
    free(e->s);
    free(e);
}

int opEdit(edit *e) { return e->op; }
int atEdit(edit *e) { return e->at; }
int toEdit(edit *e) { return e->to; }
int lengthEdit(edit *e) { return e->length; }

// Get rid of invalid UTF-8 bytes, nulls, returns and internal trailing spaces.
static int clean(int n, char *s) {
    int i = 0, j = 0;
    codePoint cp = getCodePoint();
    while (i < n) {
        nextCode(&cp, &s[i]);
        if (cp.code == UBAD) i = i + cp.length;
        else if (cp.code == '\0') i = i + cp.length;
        else if (cp.code == '\r') i = i + cp.length;
        else {
            if (cp.code == '\n') {
                while (j > 0 && s[j-1] == ' ') j--;
            }
            for (int k = 0; k < cp.length; k++) {
                s[j++] = s[i++];
            }
        }
    }
    s[j] = '\0';
    return j;
}

// Set the edit fields for any operation.
static void set(edit *e, int op, int at, int to, int n, char *s) {
    e->op = op;
    e->at = at;
    e->to = to;
    if (s == NULL) return;
    if (e->max < n) {
        while (e->max < n) e->max = e->max * 3/2;
        e->s = realloc(e->s, e->max + 1);
    }
    memcpy(e->s, s, n);
    e->s[n] = '\0';
    e->length = n;
}

void insertEdit(edit *e, int at, int n, char s[n]) {
    set(e, DoInsert, at, at+n, n, s);
    e->length = clean(n, e->s);
}

void deleteEdit(edit *e, int at, int n, char s[n]) {
    set(e, DoDelete, at, at+n, n, s);
}

void addEdit(edit *e, int at) {
    set(e, DoAdd, at, at, 0, NULL);
}

void cancelEdit(edit *e, int at) {
    set(e, DoCancel, at, at, 0, NULL);
}

void selectEdit(edit *e, int at, int to) {
    set(e, DoSelect, at, to, 0, NULL);
}

void deselectEdit(edit *e, int at, int to) {
    set(e, DoDeselect, at, to, 0, NULL);
}

void moveEdit(edit *e, int at, int to) {
    set(e, DoMove, at, to, 0, NULL);
}

void endEdit(edit *e) {
    set(e, DoEnd, 0, 0, 0, NULL);
}

void copyEdit(edit *e, char *s) {
    memcpy(s, e->s, e->length);
    s[e->length] = '\0';
}

#ifdef editTest

int main(int n, char const *args[]) {
    edit *e = newEdit();
    char s0[] = "xyz", s1[] = "xy\xFFz", s2[] = "xyz\r\n", s3[] = "xyz  \n";
    insertEdit(e, 0, 3, s0);
    assert(strcmp(e->s, "xyz") == 0);
    insertEdit(e, 0, 4, s1);
    assert(strcmp(e->s, "xyz") == 0);
    insertEdit(e, 0, 5, s2);
    assert(strcmp(e->s, "xyz\n") == 0);
    insertEdit(e, 0, 6, s3);
    assert(strcmp(e->s, "xyz\n") == 0);
    freeEdit(e);
    printf("Edit module OK\n");
    return 0;
}

#endif
