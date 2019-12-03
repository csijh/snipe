#include "edit.h"
#include "unicode.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

edit *newEdit() { return newString(); }

void freeEdit(edit *e) { freeString(e); }

int lengthEdit(edit *e) { return lengthString(e); }

void setEdit(edit *e, int op, int to) {
    setOpString(e, op);
    setToString(e, to);
}

int opEdit(edit *e) { return opString(e); }

int toEdit(edit *e) { return toString(e); }

// Get rid of invalid UTF-8 bytes, nulls, returns and internal trailing spaces.
static void clean(edit *e) {
    int n = lengthString(e);
    int i = 0, j = 0;
    codePoint cp = getCodePoint();
    while (i < n) {
        nextCode(&cp, &e[i]);
        if (cp.code == UBAD) i = i + cp.length;
        else if (cp.code == '\0') i = i + cp.length;
        else if (cp.code == '\r') i = i + cp.length;
        else {
            if (cp.code == '\n') {
                while (j > 0 && e[j-1] == ' ') j--;
            }
            for (int k = 0; k < cp.length; k++) {
                e[j++] = e[i++];
            }
        }
    }
    resizeString(e, j);
}

edit *fillEdit(edit *e, string *s) {
    int n = lengthString(s);
    resizeString(e, n);
    memcpy(e, s, n);
    e[n] = '\0';
    if (opString(e) == DoInsert) clean(e);
    return e;
}

#ifdef editTest

int main(int n, char const *args[]) {
    string *s = newString();
    edit *e = newEdit();
    fillEdit(e, fillString(s, "xyz"));
    assert(strcmp(e, "xyz") == 0);
    fillEdit(e, fillString(s, "xy\xFFz"));
    assert(strcmp(e, "xyz") == 0);
    fillEdit(e, fillString(s, "xyz\r\n"));
    assert(strcmp(e, "xyz\n") == 0);
    fillEdit(e, fillString(s, "x   \nyz"));
    assert(strcmp(e, "x\nyz") == 0);
    freeEdit(e);
    freeString(s);
    printf("Edit module OK\n");
    return 0;
}

#endif
