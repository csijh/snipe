// Primitive edit operations. Free and open source. See licence.txt.
#include "array.h"

// Each user action is converted into a sequence of edits to a document,
// including automatic adjustments such as indenting. The state of a document
// consists of its text, and an array of cursors and selectors. The array is in
// order of position in the text, so cursors and selectors can be specified by
// position rather than by index. Edits are relative to a current position in a
// document, and each edit may result in a new current position. The current
// position is often, but not always, a cursor position.

// Ins inserts a given string at a given new position. There must be no
// selection there. The cursor at the position, if any, ends up to the right of
// the insertion. Del deletes from the current position to the given new
// position. There must be no selection there. If there is a cursor, it must be
// at the right hand end of the deletion, so that insertion and deletion are
// inverses for the benefit of undo/redo.

// Move moves the current cursor to a new position. Select sets a selector at
// the current position and moves the cursor to a new position. Deselect moves
// the current cursor to its selector at the given position and deletes the
// selector. New creates a new cursor. Old deletes the current cursor.

enum edit {
    Ins, Del, Move, Select, Deselect, New, Old
};

// An edit is stored as a string.
typedef string edit;

// Synonyms are provided for the string functions in the array module.
edit newEdit();
void freeEdit(edit e);
int lengthEdit(edit e);
void *resizeEdit(edit e, int n);
void clearEdit(edit e);
int opEdit(edit e);
void setOpEdit(edit e, int op);
void setAtEdit(edit e, int at);
int atEdit(edit e);
