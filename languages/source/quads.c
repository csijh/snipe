// Snipe language compiler. Free and open source. See licence.txt.
#include "quads.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

static quad *newQuad(tag *l, char op, tag *r, tag *t) {
    struct quad *q = malloc(sizeof(quad));
    *q = (quad) { .left = l, .op = op, .right = r, .override = t };
    return q;
}

static bool eqQuad(quad *q, tag *l, char op, tag *r, tag *t) {
    return q->left == l && q->op == op && q->right == r && q->override == t;
}

// Find an existing quad or create a newly allocated one.
quad *findQuad(quad ***qsp, tag *l, char op, tag *r, tag *t) {
    quad **qs = *qsp;
    int i;
    for (i = 0; qs[i] != NULL; i++) if (eqQuad(qs[i],l,op,r,t)) return qs[i];
    quad *q = newQuad(l,op,r,t);
    addQuad(qsp, q);
    return q;
}

void freeQuad(quad *t) { free((struct quad *) t); }

#ifdef quadsTest

int main(int argc, char const *argv[]) {
    return 0;
}

#endif
