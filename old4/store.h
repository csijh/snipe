// Snipe editor. Free and open source, see licence.txt.
#include <stdbool.h>

// A store holds an array of objects (e.g. bytes or tokens or brackets or undo
// items) split into lines. The lines are indexed by row (i.e. zero-based line
// number). An operation may cause a reallocation or may move data within the
// store, so any pointer obtained into the array is volatile, i.e. only valid
// until the next operation.
typedef struct store store;

// A position is a row and column (zero-based line number and zero-based index
// within the line).
typedef struct position { int row, col; } position;

// Create an empty store for objects of the given size.
store *newStore(int size);

// Free a store, its data, and its line information.
void freeStore(store *b);

// The number of lines of objects in the store.
int rows(store *b);

// Find the length of a line.
int cols(store *b, int row);

// Provide a volatile pointer to a given line.
void *fetch(store *b, int row);

// Prepare for an insert of n objects at a given position. Return a volatile
// pointer to the subarray of n objects to be filled in.
void *insert(store *b, position p, int n);

// Report a deletion of n objects backward from position p. The deleted items
// should already have been copied out, if desired.
void *delete(store *b, position p, int n);

// Split at the given position, i.e. add a line boundary there. The row number
// may be equal to the number of rows, with column zero, to add a line at the
// end of the store.
void split(store *b, position p);

// Join lines, i.e. remove a line boundary at the given row position.
void join(store *b, int row);
