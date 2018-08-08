// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include <stdbool.h>

// TODO: Elide successive 1-char insertions.

// A history object is an undo or redo store.
struct history;
typedef struct history history;

// An opcode specifies an insertion or deletion. The four deletion variants
// allow the cursor to be reconstructed accurately when the edit is undone.
enum opcode { Ins, DelRight, DelLeft, CutRight, CutLeft };
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
