// Snipe language compiler. Free and open source. See licence.txt.
#include "quads.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

struct quad {
    tag *left, *right, *override;
    char op;
    int row;
};

quad **newQuads() { return (quad **) newPointers(); }
int countQuads(quad *list[]) { return countPointers((void **)list); }
void addQuad(quad ***listp, quad *t) { addPointer((void ***)listp, t); }

static quad *newQuad(int row, tag *l, char op, tag *r, tag *t) {
    struct quad *q = malloc(sizeof(quad));
    *q = (quad) {
        .index = -1, .left = l, .op = op, .right = r, .override = t, .row = row
    };
    return q;
}

static bool eqQuad(quad *q, tag *l, char op, tag *r, tag *t) {
    return q->left == l && q->op == op && q->right == r && q->override == t;
}

// Find an existing quad or create a newly allocated one.
quad *findQuad(quad ***qsp, int row, tag *l, char op, tag *r, tag *t) {
    quad **qs = *qsp;
    int i;
    for (i = 0; qs[i] != NULL; i++) if (eqQuad(qs[i],l,op,r,t)) return qs[i];
    quad *q = newQuad(row, l, op, r, t);
    addQuad(qsp, q);
    return q;
}

// Mark the tags in a quad.
static void mark(quad *q) {
    if (q->op != '=') return;
    if (tagChar(q->override) == MORE) {
        setBracket(q->left, q->row);
        setBracket(q->right, q->row);
    }
    else {
        setDelimiter(q->left, q->row);
        setDelimiter(q->right, q->row);
    }
    setOpener(q->left, q->row);
    setCloser(q->right, q->row);
}

void markTags(quad **qs) {
    for (int i = 0; qs[i] != NULL; i++) mark(qs[i]);
}

#ifdef quadsTest

int main(int argc, char const *argv[]) {
    return 0;
}

#endif
