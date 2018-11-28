// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// The op structure stores a single atomic insertion or deletion as an object,
// to be offered to all relevant components (text, style, lines, history,
// brackets, indents, cursors). The data is valid only until the next op.
struct op;
typedef struct op op;

// Create or free a reusable op structure.
op *newOp();
void freeOp(op *o);

// Flags describing the op. Fix specifies an automatic adjustment rather
// than a user edit (which affects the undo history). Del specifies a deletion
// rather than an insertion, which is the default. If a cursor is involved, Left
// specifies a leftward direction compared to the cursor (so typing a character
// is a Left insertion, and Backspace is a Left deletion, and undo restores a
// cursor on the right.) Sel specifies that the deleted text was selected (so
// Undo restores the selection.)
extern const int Fix, Del, Left, Sel;

// Fill in the structure. The flags can be any meaningful combination of the
// above, s is the text for an insertion, last says whether it is the last
// op of a multi-cursor edit.
void setOp(op *o, int flags, int at, int n, char s[n], bool last);

// When there is a deletion, fill in the deleted text.
void setDeletion(op *o, char *s);

// Get the data out of an op structure.
int flagsOp(op *o);
int atOp(op *o);
int lengthOp(op *o);
char *textOp(op *o);
bool lastOp(op *o);
