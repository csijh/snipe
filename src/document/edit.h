// History and primitive edit operations. Free and open source. See licence.txt.

// Each user action becomes a sequence of edits to a document. The edits include
// automatic adjustments such as re-indenting. The state of a document consists
// of its text, and a set of cursors. The edits are based on a current position,
// not necessarily related to a cursor, and a current cursor index. The edits
// and their rules are designed so that the edits are inverses of each other for
// undo/redo, and that an undo restores cursors as well as text. The edits are:

// Goto(p): change current position.

// Insert(s): insert string s before current position. A cursor base or mark at
// or after the current position is adjusted.

// Delete(s): delete string s before current position. There must be no cursor
// base or mark in the range of the deletion, except at the current position.

// SetCursor(c): change current cursor. For n cursors, 0 <= c <= n with c == n
// only immediately preceding an AddCursor.

// AddCursor(): insert a cursor at the current position. This must preserve the
// ordering of the cursor bases.

// DelCursor(): delete the current cursor. The cursor must not have a selection.
// If the last cursor is deleted, this must be followed by a SetCursor.

// MoveCursor: move the current cursor to the current position. There must be no
// selection and the order of the cursor bases must be preserved.

// MoveBase: move the base of the current cursor to the current position,
// preserving the order of the cursor bases.

// MoveMark: move the mark of the current cursor to the current position.

enum edit {
    Goto, DoInsert, DoDelete, SetCursor, AddCursor, DelCursor, MoveCursor,
    MoveBase, MoveMark
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
