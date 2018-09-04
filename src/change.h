// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// Represent a single insertion or deletion as a data structure, so it can be
// passed to all relevant components (text, style, lines, history, brackets,
// cursors). A change is an edit caused by the user, or an adjustment which
// restores an invariant such as correct indenting. The difference is that any
// adjustments following an edit do not need to be stored explicitly in the
// history. Instead, an undo operation is followed by adjustments to
// re-establish the invariant, which automatically reconstructs the state.

// The change structure holds the data for an edit or adjustment. The data held
// is assumed to be valid only until the next change.
struct change;
typedef struct change change;

// Define the op type, without including the op.h header.
typedef int op;

// Create or free a reusable change structure.
change *newChange();
void freeChange(change *c);

// Fill in the structure to represent an edit or adjustment.
void setChange(change *c, op o, int n, char s[n], bool last);

// When there is a deletion, fill in the deleted text.
void setDeletion(change *c, char *s);

// Get the data out of a change structure.
op changeOp(change *c);
int changeLength(change *c);
char *changeText(change *c);
bool changeLast(change *c);
