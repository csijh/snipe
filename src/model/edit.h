// Primitive edit operations. Free and open source. See licence.txt.
#include "array.h"

// Each user action is converted into a sequence of edits to a document,
// including automatic adjustments such as indenting. The state of a document
// consists of its text, and an array of cursors and selectors. The array is in
// order of position in the text, so cursors and selectors can be specified by
// position rather than by index.

// Ins(at,s) inserts string s at a given position. There must be no selection
// there. The cursor at the position, if any, ends up to the right of the
// insertion. Del(at,by) deletes a string backward from a given position. There
// must be no selection there. If there is a cursor, it must be at the right
// hand end of the deletion, so that insertion and deletion are inverses for the
// benefit of undo/redo.

// Move(at,by) moves the cursor at a given position by a given amount.
// Select(at,by) moves a cursor by a given amount, leaving a selector at the
// original position. Deselect(at,by) moves a cursor at a given position by a
// given amount to its selector and deletes the selector. New(at) creates a new
// cursor at a given position. Old(at) deletes the cursor at a given position.

enum edit {
    Ins, Del, Move, Select, Deselect, New, Old
};

// Ins/Del at, string-len
// New at
// Old at
// Move cursor(=at) by(!=len)
// Select cursor by
// Deselect cursor by

// An edit is stored as a string.
typedef string edit;
// struct: length, max, op(sh), cursor i, by, string

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
void setByEdit(edit e, int by);
int byEdit(edit e);
void setStringEdit(edit e, char *s);
int stringEdit(edit e);
