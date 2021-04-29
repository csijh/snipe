// Snipe language compiler. Free and open source. See licence.txt.
#include "tags.h"

// A quad is a pair of tags, each a bracket or delimiter, with a match operator,
// and a tag for overriding the tokens between them if they match or mismatch.
struct quad;
typedef struct quad quad;

// Lists of quads.
quad **newQuads();
int countQuads(quad *list[]);
void addQuad(quad ***listp, quad *t);

// Find an existing quad or create a newly allocated one, with line number.
quad *findQuad(quad ***qsp, int row, tag *l, char op, tag *r, tag *t);

// Use quads to mark tags as brackets, delimiters, openers, closers. A quad such
// as (=)- with no override of intervening tokens marks () as brackets. Quads
// '='= and <=>* mark '<> as delimiters. A tag can't be both a bracket and
// delimiter. A quad (=)- marks ( as an opener and ) as a closer. A bracket
// can't be both. A quad '='= marks ' as both an opener and closer.
void markTags(quad **qs);
