// The Snipe editor is free and open source, see licence.txt.
#include "change.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct change {
    int flags;
    int n;
    char *s;
    bool last;
};

// Flags are stored both ways round for simplicity of testing.
const int
    Fix = 0x1, Edit = 0x10, Ins = 0x2, Del = 0x20,
    Left = 0x4, Right = 0x40, Sel = 0x8, NoSel = 0x80;

change *newChange() {
    return malloc(sizeof(change));
}

void freeChange(change *c) {
    free(c);
}

void setChange(change *c, int flags, int n, char s[n], bool last) {
    assert(((flags >> 4) & flags) == 0);
    *c = (change) {.flags=flags, .n=n, .s=s, .last=last};
}

extern inline void setDeletion(change *c, char *s) {
    c->s = s;
}

int changeFlags(change *c) { return c->flags; }
int changeLength(change *c) { return c->n; }
char *changeText(change *c) { return c->s; }
bool changeLast(change *c) { return c->last; }

#ifdef changeTest

// No testing.
int main() {
    printf("Change module OK\n");
}

#endif
