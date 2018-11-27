// The Snipe editor is free and open source, see licence.txt.

// Store an insertion or deletion, so it can be offered to all relevant
// components (text, style, lines, history, brackets, indents, cursors).
#include <stdbool.h>

// The change structure holds the data for a single atomic insertion or
// deletion. The data held is assumed to be valid only until the next change.
struct change;
typedef struct change change;

// Create or free a reusable change structure.
change *newChange();
void freeChange(change *c);

// Flags describing the change. Use at most one of each pair. Fix/Edit specifies
// an adjustment or a user edit, which affects undo history. Ins/Del say whether
// it is an insertion or deletion. For a user deletion, Left/Right give the
// direction of deletion, i.e. the cursor was at the right/left end of the text
// before deletion. Sel/NoSel say whether the text was selected. Left|Sel means
// the selection was made left to right leaving the cursor at the right end.
extern const int Fix, Edit, Ins, Del, Left, Right, Sel, NoSel;

// Fill in the structure. The flags can be any meaningful combination of the
// above, s is the text for an insertion, last says whether it is the last
// change of a multi-cursor edit.
void setChange(change *c, int flags, int n, char s[n], bool last);

// When there is a deletion, fill in the deleted text.
void setDeletion(change *c, char *s);

// Get the data out of a change structure.
int changeFlags(change *c);
int changeLength(change *c);
char *changeText(change *c);
bool changeLast(change *c);
