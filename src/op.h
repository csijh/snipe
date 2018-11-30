// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// The op structure stores a single insertion, deletion or cursor movement as an
// object, to be offered to all relevant components (text, lines, style, undo,
// brackets, indents, cursors). The data is valid only until the next op.
struct op;
typedef struct op op;

// Create or free a reusable op structure.
op *newOp();
void freeOp(op *o);

// Flags describing the op. Fix specifies an automatic adjustment rather than a
// user edit (which affects the undo history). Del specifies a deletion rather
// than an insertion, which is the default. If a cursor is involved, Left
// specifies a leftward direction compared to the cursor (so typing a character
// is a Left insertion, and Backspace is a Left deletion, and undo restores a
// cursor on the right.) Sel specifies that the deleted text was selected (so
// Undo restores the selection.) Multi says whether this is an op in a
// multi-cursor sequence, other than the last.
extern const int Fix, Del, Left, Sel, Multi;

// Fill in the structure, with s as the text for an insertion or deletion. If n
// is 0, the op is a cursor movement.
void setOp(op *o, int flags, int at, int n, char s[n]);

// When there is a deletion, fill in the deleted text.
void setDeletion(op *o, char *s);

// Get the data out of an op structure.
int flagsOp(op *o);
int atOp(op *o);
int lengthOp(op *o);
char *textOp(op *o);
