// History, undo, redo. Free and open source. See licence.txt.
#include "list.h"
#include <stdbool.h>

// A history object records edits for undo or redo.
struct history;
typedef struct history history;

// Each user action becomes a sequence of edits to a document. The edits include
// automatic adjustments such as re-indenting. The state of a document consists
// of its text, its cursors, and the index of the current cursor. The edits and
// their restrictions are designed so that the edits are inverses of each other.
struct edit { int op, at, n; char *s; };
typedef struct edit edit;

// Codes for edit operations. Each operation includes a current position in the
// text, 'at', which the history object tracks. Insert(n,s) inserts string s of
// length n. Any cursor end at the insertion position is implicitly moved to the
// end of the insertion. Delete(n,s) deletes a preceding string s of length n.
// There must be no cursor end in the range of the deletion, except at the
// deletion position. SetCursor(n) makes the n'th cursor current. AddCursor()
// inserts a new cursor after the current one. This must preserve the ordering
// of the cursors. DelCursor() deletes the cursor after the current one. The
// cursor must not have a selection. MoveCursor() moves the current cursor.
// There must be no selection and the order of the cursors must be preserved.
// MoveBase() moves the base of the current cursor, preserving the order of the
// cursors. MoveMark() moves the mark of the current cursor. End() marks the end
// of a user action.
enum op {
    Insert, Delete, SetCursor, AddCursor, CutCursor, MoveCursor, MoveBase,
    MoveMark, End
};

// Add an edit to the history.
void pushEdit(history *h, int op, int at, int n, char *s);

// Get the most recent edit, inverted ready for execution. This should be
// repeated until an End edit is reached.
edit undo(history *h);

// Get the most recent undone action, ready for re-execution. This should be
// repeated until an End edit is reached.
edit redo(history *h);

// ----------
// TODO: Elide successive 1-char insertions.


// An opcode specifies an insertion, deletion with no reference to a cursor,
// deletion with a cursor at the left or right end, or deletion of a
// left-to-right or right-to-left selection. These allow the cursor to be
// reconstructed when the edit is undone.
enum opcode { Ins, Del, DelL, DelR, DelLR, DelRL };
typedef int opcode;

// Create a new history stack.
history *newHistory(void);

// Free a history object and its contents.
void freeHistory(history *h);

// Push an edit into the history. Op specifies the type of operation, 'at' is
// the position of the edit, n is the number of bytes inserted or deleted and,
// for a deletion, s is the deleted text (which is only valid until the next
// edit). The last flag allows multiple cursor edits to be grouped.
void pushEdit(history *h, opcode op, int at, int n, char *s, bool last);

// Pop an edit from the history into the given variables. If there are no edits,
// the field n is zero.
void popEdit(history *h, opcode *op, int *at, int *n, char *s, bool *last);

// Remove all the entries.
void clear(history *h);
