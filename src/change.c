// The Snipe editor is free and open source, see licence.txt.
#include "change.h"
#include "op.h"
#include <stdio.h>
#include <stdlib.h>

struct change {
    op o;
    int n;
    char *s;
    bool last;
};

change *newChange() {
    return malloc(sizeof(change));
}

void freeChange(change *c) {
    free(c);
}

extern inline void setChange(change *c, op o, int n, char s[n], bool last) {
    *c = (change) {.o=o, .n=n, .s=s, .last=last};
}

extern inline void setDeletion(change *c, char *s) {
    c->s = s;
}

extern inline op changeOp(change *c) { return c->o; }
int changeLength(change *c) { return c->n; }
char *changeText(change *c) { return c->s; }
bool changeLast(change *c) { return c->last; }

#ifdef test_change

// No testing.
int main() {
    printf("Change module OK\n");
}

#endif
