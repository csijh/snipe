// History, undo, redo. Free and open source. See licence.txt.
#include <stdbool.h>

// A history object records edits for undo or redo. Each user action becomes a
// sequence of edits, including automatic adjustments such as re-indenting. The
// state of a document consists of its text, its cursors, and the index of the
// current cursor. The edits and their restrictions are designed so that the
// edits are inverses of each other.
struct history;
typedef struct history history;

// Remove all the entries.
void clearHistory(history *h);

// Save an insertion of a string s of length n at p. Any cursor end at the
// insertion position is implicitly moved to the end of the insertion.
void saveInsert(history *h, int p, int n, char const *s);

// Save a deletion of a string s of length n, before position p. There must be
// no cursor end in the range of the deletion, except at the deletion position.
void saveDelete(history *h, int p, int n, char const *s);

// Save a selection of the n'th cursor as current.
void saveSetCursor(history *h, int n);

// Save an addition of a new cursor after the current one at p. The ordering of
// the cursors is preserved.
void saveAddCursor(history *h, int p);

// Save a deletion of the cursor after the current one. The cursor doesn't have
// a selection.
void saveCutCursor(history *h);

// Save a movement of the current cursor to p. The cursor doesn't have a
// selection and the order of the cursors is preserved.
void saveMoveCursor(history *h, int p);

// Save a movement of the base of the current cursor, preserving the order of
// the cursors.
void saveMoveBase(history *h, int p);

// Save a movement of the mark of the current cursor.
void saveMoveMark(history *h, int p);

// Record the end of the current user action.
void saveEnd(history *h);

// An edit retrieved for undo or redo is stored in an edit structure. The string
// s, if any, is only valid until the next edit. The last flag means the edit is
// the last of an undo or redo sequence corresponding to a user action.
struct edit { int op, at, n; bool last; char const *s; };
typedef struct edit edit;

// Codes for edit operations.
enum op {
    Insert, Delete, SetCursor, AddCursor, CutCursor, MoveCursor, MoveBase,
    MoveMark, End
};

// Get the most recent edit, inverted ready for execution. This should be
// repeated until the last flag is set.
edit undo(history *h);

// Get the most recent undone action, ready for re-execution. This should be
// repeated until the last flag is set.
edit redo(history *h);
