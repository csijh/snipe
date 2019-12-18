// Primitive edit operations. Free and open source. See licence.txt.

// Each user action becomes a sequence of edits to a document. The edits include
// automatic adjustments such as re-indenting. The state of a document consists
// of its text, and a set of cursors with possible selectors. The edits and
// their rules are designed so that the edits are inverses of each other for
// undo, and that an undo restores cursors as well as text.

// DoInsert(at,s) is an insertion of a string before the given position. If
// there is a cursor there, it moves to the end of the insertion. DoDelete(at,s)
// is a deletion of a string up to the given position. The deleted text mustn't
// include a cursor other than at the end. The string is filled in from the text
// when the edit is created.

// DoAdd(at) adds a new cursor at the given position. DoCancel(at) removes a
// cursor at the given position. DoSelect(at,to) moves the cursor from the given
// position and selects the text covered. DoDeselect(at,to) moves the cursor at
// the current position to its selector, cancelling it. DoMove(at,to) moves the
// cursor at the given position (self-inverse). DoEnd(at) marks the end of the
// edits for one user action, and sets the current cursor position.

// AddCursor(at), DelCursor(at) must be no selection (so inverses)
// MoveCursor(from,to) must be no selection (so self-inverse)
// MoveEnd(B,from,to) can be used to select/deselect, base is unique
// MoveEnd(LR,from,to) L/R needed because marker position isn't unique

// SetCursor(i): make the i'th cursor current (or Set(n) for n cursors).
// -- store delta-i because self-inverse.
// AddCursor(at): insert a new cursor, in base order, which becomes current
// -- store delta-at, so know where to undo to
// DelCursor(at): delete current cursor, which has no selection and is at 'at'.
// MoveCursor(to): move current cursor, which has no selection. (delta)
// MarkCursor(to): move just the marker, to anywhere. (delta)
// Ins(s): insert s at current cursor, which has no selection.
// Del(s): delete back from current cursor, which has no selection.

// Then: don't have to maintain non-overlap, but can act first and
// collapse afterwards. Must create or move cursor temporarily to ins/del. Must
// move marker to deselect. How add n'th cursor? Add after maybe? Or set(n)?

enum editOp {
    DoInsert, DoDelete, DoAdd, DoCancel, DoSelect, DoDeselect, DoMove, DoEnd
};

struct edit;
typedef struct edit edit;

// Create or free an edit structure.
edit *newEdit();
void freeEdit(edit *e);

// Get an edit's operation, 'at' position, 'to' position, or length of text.
int opEdit(edit *e);
int atEdit(edit *e);
int toEdit(edit *e);
int lengthEdit(edit *e);

// Set up an edit. For insertion, the string s is cleaned in place to remove
// invalid UTF-8 bytes, nulls, returns, or trailing spaces.
void insertEdit(edit *e, int at, int n, char s[n]);
void deleteEdit(edit *e, int at, int n, char s[n]);
void addEdit(edit *e, int at);
void cancelEdit(edit *e, int at);
void selectEdit(edit *e, int at, int to);
void deselectEdit(edit *e, int at, int to);
void moveEdit(edit *e, int at, int to);
void endEdit(edit *e);

// Get the string from an insertion or deletion, valid only during the edit.
char const *stringEdit(edit *e);

// Copy the string from an insertion or deletion to s.
void copyEdit(edit *e, char *s);
