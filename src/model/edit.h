// Primitive edit operations. Free and open source. See licence.txt.
#include "array.h"

// Each user action is converted into a sequence of edits to a document. The
// edits include automatic adjustments such as re-indenting. The state of a
// document consists of its text, a set of cursors with possible selections, and
// a current cursor. Each edit operates at the current cursor, and takes an
// argument 'to' representing the final target position of the current cursor:

// DoInsert(to) insert text before the current cursor.
// DoDelete(to) delete text to a position before the current cursor.
// DoMove(to) moves the current cursor.
// DoSwap(to) changes the current cursor to the one at the given position.
// DoSelect(to) moves the cursor and selects the text covered.
// DeSelect(to) moves to the other end of the cursor's selection, cancelling it.
// DoAdd(to) adds a new cursor at the given position.
// DoCancel(to) removes the current cursor, and makes another current.

enum edit {
    DoInsert, DoDelete, DoMove, DoSwap, DoSelect, DeSelect, DoAdd, DoCancel
};

// An edit is stored as a string.
// TODO: make opaque, copy in/out.
typedef string edit;

// Synonyms for newString, freeString, lengthString.
edit *newEdit();
void freeEdit(edit *e);
int lengthEdit(edit *e);

// Set up an edit to represent the given operation.
void setEdit(edit *e, int op, int to);

// Find an edit's operation or 'to' position.
int opEdit(edit *e);
int toEdit(edit *e);

// Fill in the text to be inserted or deleted. For insertion, the text is
// cleaned. Return the possibly reallocated edit.
edit *fillEdit(edit *e, string *s);

// TODO: maybe deletion text has to be filled in early. Maybe fillEdit is used
// INSTEAD of setEdit for ins/del. Tell undo about end of edits.
