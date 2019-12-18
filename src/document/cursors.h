// Cursors and selections. Free and open source. See licence.txt.

// A label L means a selection is leftward, with the marker at the left end. A
// label R means the marker is at the right end, and N means no selection.
enum label { L = -1, N = 0, R = 1 };

// A cursor covers a range of text (from <= to), with (from == to) if there is
// no selection. The label specifies the direction of the selection. There is a
// remembered column for up/down movement.
struct cursor { int from, to, label, col; };
typedef struct cursor cursor;

// Multiple cursors are supported. The cursors are kept in order of position in
// the text, with one cursor being current. Cursors don't overlap, except for
// the current cursor during a drag operation. Cursors can only touch if there
// is no visual ambiguity, i.e. they both have selections and at least one has a
// marker at the position where they touch.
struct cursors;
typedef struct cursors cursors;

// ----------
// A leftward move or delete on multiple cursors is done left to right. An
// insertion, or rightward move or delete, is done right to left. That minimises
// the effects of cursors on each other.

// This module removes trailing spaces or trailing blank lines on every edit,
// except where needed to maintain a cursor position. Cursor changes are passed
// to the history object. Text changes are passed to the text object, which
// passes them to the history and lines objects.

// Return the number of cursors.
int nCursors(cursors *cs);

// Return the i'th cursor.
cursor getCursor(cursors *cs, int i);

// Add a cursor.
void addCursor(cursors *cs, int at);

// Remove a cursor.
void cancelCursor(cursors *cs, int at);

// Move the i'th cursor.
void moveCursor(cursors *cs, int i, int to);

// Make a selection by moving the i'th cursor, leaving a selector behind.
void selectCursor(cursors *cs, int i, int to);

// Insert text at the i'th cursor.
void insertCursor(cursors *cs, int i, char *s);

// Delete text from the i'th cursor to the given position.
void cutCursor(cursors *cs, int i, int to)

// Collapse overlapping cursors. (Call after each multi-cursor edit, or when
// dropping a cursor after a drag).
void collapseCursors(cursors *cs);
