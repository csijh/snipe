// The Snipe editor is free and open source, see licence.txt.
#include "op.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct op {
    int flags;
    int at;
    int n;
    char *s;
};

const int Fix = 0x1, Del = 0x2, Left = 0x4, Sel = 0x8, Multi = 0x10;

op *newOp() {
    return malloc(sizeof(op));
}

void freeOp(op *o) {
    free(0);
}

void setOp(op *o, int flags, int at, int n, char s[n]) {
    *o = (op) {.flags=flags, .at=at, .n=n, .s=s};
}

extern inline void setDeletion(op *o, char *s) {
    o->s = s;
}

int flagsOp(op *o) { return o->flags; }
int atOp(op *o) { return o->at; }
int lengthOp(op *o) { return o->n; }
char *textOp(op *o) { return o->s; }

#ifdef opTest

// No testing.
int main() {
    printf("Op module OK\n");
}

#endif
