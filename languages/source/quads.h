// Snipe language compiler. Free and open source. See licence.txt.
#include "tags.h"

// A quad is a pair of brackets or delimiters, with an operator between them,
// and a tag used to override the tokens between them if they match or mismatch.
struct quad {
    tag *left, *right, *override;
    char op;
};
typedef struct quad const quad;

// Find an existing quad or create a newly allocated one.
quad *findQuad(quad ***qsp, tag *l, char op, tag *r, tag *t);

// Deallocate a quad.
void freeQuad(quad *t);
