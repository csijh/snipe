// History with undo and redo. Free and open source. See licence.txt.
#include <stdbool.h>
#include "text.h"

// A history object records edits for undo or redo. Each user action becomes a
// sequence of edits, including automatic adjustments such as re-indenting. The
// edits and their restrictions are designed so that the edits are invertible.
struct history;
typedef struct history history;

// Create a new history object.
history *newHistory();

// Free up a history object.
void freeHistory(history *h);

// Remove all the entries.
void clearHistory(history *h);

// Save an insertion, possibly including a deletion of spaces, or a deletion,
// or some other operation such as a cursor change.
void saveEdit(history *h, edit *e);

// Record the end of the current user action.
void saveEnd(history *h);

// Check whether the next undo was the first in its user action.
bool undoIsLast(history *h);

// Check whether the next redo was the last in its user action.
bool redoIsLast(history *h);

// Get the most recent edit. This should be repeated until undoIsLast returns
// true. The caller is responsible for inverting and executing the operation,
// without recording it in the history.
edit undo(history *h, edit *e);

// Get the most recent undone operation, ready for re-execution. This should be
// repeated until redoIsLast returns true. The caller is responsible for
// executing the operation, without recording it in the history.
edit redo(history *h);

/*
// Save an insertion of a string s of length n at a given position, relative to
// the current cursor. Any cursor end at that position is implicitly moved to
// the end of the inserted text.
void saveInsert(history *h, int by, int n, char const *s);

// Save a deletion of a string s of length n, before a given position, relative
// to the current cursor. There must be no cursor end in the range of the
// deletion, except at that position.
void saveDelete(history *h, int by, int n, char const *s);

// Save an addition of a new cursor after the current one at a given position,
// relative to the current cursor. The ordering of the cursors is preserved.
void saveAddCursor(history *h, int by);

// Save a deletion of the cursor after the current one. The cursor was at the
// given position, relative to the current cursor, and doesn't have a selection.
void saveCutCursor(history *h, int by);

// Save a change of cursor, by a relative index.
void saveSetCursor(history *h, int by);

// Save a relative movement of the current cursor. The cursor doesn't have a
// selection and the order of the cursors is preserved.
void saveMoveCursor(history *h, int by);

// Save a relative movement of the base of the current cursor, independent of
// the mark, preserving the order of the cursors.
void saveMoveBase(history *h, int by);

// Save a relative movement of the mark of the current cursor, independent of
// the base.
void saveMoveMark(history *h, int by);

// An edit, retrieved for undo or redo. The string s, if any, is only valid
// until the next edit. The flag means the edit is the last of an undo or redo
// sequence corresponding to a user action.
struct edit { bool last; int op, by, n; char const *s; };
typedef struct edit edit;

// Edit operations as opcodes.
enum op {
    Insert, Delete, SetCursor, AddCursor, CutCursor, MoveCursor, MoveBase,
    MoveMark, End
};
*/
