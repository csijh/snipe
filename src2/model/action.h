// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// Primitive actions are all relative to a current cursor:

//    Go p: neutral
//    Insert s: to left of cursor, if there is one (no selection at the time)
//    Delete s: to left of cursor, if there is one (so inverse of insert)
//    Move: move current cursor to p (self inverse, no crossing)
//    Select: put selector at current cursor and move cursor to p
//    Deselect: move cursor to selector at p and cancel selector (inverse)
//    Swap: swap to a different cursor at p (self-inverse)
//    New: add a new cursor at p and swap to it
//    Old: remove the current cursor and swap to the one at p

//    Begin: start of group of edits, = Move 0
//    End: end of group of edits, = Swap 0

// Two bits: Go/Ins/Del/Mark
//    Go: +point
//    Ins/Del: +str (max 63?)
//    Mark: 6 bits hold Move/Sel/Desel/Swap/New/Old

// Maintain a queue of actions. Define action structure. What if it extends?
// Internal. What if an insert/delete extends? (A) extend it somehow or (B) have
// a fixed limit and fragment. (B) starting to look better, except maybe for
// bulk. Max 31 means length is in op byte. For N byte paste, that's N*32/31,
// but could merge in history! Which makes 31 silly.

// In history: three bits for op, five for unsigned length or signed offset.
// What about different length options?

// An action structure stores a single primitive editing operation. An insertion
// or deletion is before the cursor, so may require a cursor action first. A
// Move implicitly deselects. A Drag implicitly creates a selection. An Add adds
// a new cursor. A Switch chooses a different cursor. A Cancel removes a
// cursor.
enum action {
    Ins, Del, Move, Drag, Add, Switch, Cancel
};

// insertion, deletion or cursor movement as
// an object, to be offered to all relevant components (text, lines, style,
// undo, brackets, indents, cursors). The data is valid only until the next
// action.
struct action;
typedef struct action action;

// Create or free a reusable action structure.
action *newAction();
void freeAction(action *o);

// Flags describing the action. Fix specifies an automatic adjustment rather than a
// user edit (which affects the undo history). Del specifies a deletion rather
// than an insertion, which is the default. If a cursor is involved, Left
// specifies a leftward direction compared to the cursor (so typing a character
// is a Left insertion, and Backspace is a Left deletion, and undo restores a
// cursor on the right.) Sel specifies that the deleted text was selected (so
// Undo restores the selection.) Multi says whether this is an action in a
// multi-cursor sequence, other than the last.
extern const int Fix, Del, Left, Sel, Multi;

// Fill in the structure, with s as the text for an insertion or deletion. If n
// is 0, the action is a cursor movement.
void setAction(action *o, int flags, int at, int n, char s[n]);

// When there is a deletion, fill in the deleted text.
void setDeletion(action *o, char *s);

// Get the data out of an action structure.
int flagsAction(action *o);
int atAction(action *o);
int lengthAction(action *o);
char *textAction(action *o);
