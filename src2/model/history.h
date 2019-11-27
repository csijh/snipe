// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include <stdbool.h>

// TODO: Elide successive 1-char insertions.

// A history object is an undo or redo store.
struct history;
typedef struct history history;

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
