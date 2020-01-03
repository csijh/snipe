// Cursors and selections. Free and open source. See licence.txt.

// A cursor has a base and a mark, which are equal if there is no selection.
// There is a remembered column for up/down movement. The cursors are kept in
// order of position of their bases in the text, with one cursor being current.
// After a user action, cursors are collapsed so that they don't overlap, and
// they only touch if there is no visual ambiguity, i.e. they both have
// selections and it is not the two bases which touch. During drag actions, the
// cursors are not collapsed until the drop action.
struct cursors;
typedef struct cursors cursors;

// Allocate or free a set of cursors.
cursors *newCursors();
void freeCursors(cursors *cs);

// Return the number of cursors.
int nCursors(cursors *cs);

// Return the index of the current cursor.
int currentCursor(cursors *cs);

// Set the i'th cursor as current.
void setCursor(cursors *cs, int i);

// Add a cursor after the current one.
void addCursor(cursors *cs, int at);

// Remove the cursor after the current one.
void cutCursor(cursors *cs);

// Move the current cursor (which has no selection).
void moveCursor(cursors *cs, int to);

// Move the current cursor's mark.
void selectCursor(cursors *cs, int to);

// Move the current cursor's base.
void rebaseCursor(cursors *cs, int to);

// Get the base and mark, or left and right, or column, of the current cursor.
int cursorBase(cursors *cs);
int cursorMark(cursors *cs);
int cursorLeft(cursors *cs);
int cursorRight(cursors *cs);
int cursorCol(cursors *cs);
