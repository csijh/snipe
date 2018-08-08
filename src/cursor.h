// The Snipe editor is free and open source, see licence.txt.
#include "list.h"

// Handle cursors, the effects of edit actions on them, and their effects on the
// display. A cursor holds a caret position and optionally a selection and a
// remembered column. To support multiple cursors, the first cursor may contain
// a chain of further cursors.
struct cursors;
typedef struct cursors cursors;

// TODO: consider regarding a gap as attached to a nearby token (L,R,N,P)

// Create a set of cursors containing a single initial cursor.
cursors *newCursors(ints *lines, chars *styles);

// Free up the cursors.
void freeCursors(cursors *cs);

// Return the number of cursors.
int countCursors(cursors *cs);

// Return the position of the ith cursor, or its selection.
int cursorAt(cursors *cs, int i);
int cursorFrom(cursors *cs, int i);

// Update cursors as a result of an insertion (n>0) or deletion (n<0) at p.
void updateCursors(cursors *cs, int p, int n);

// Find the maximum row containing a cursor, so that scanning can be made up to
// date up to that point, before a style-based cursor action.
int maxRow(cursors *cs);

// Check for overlapping cursors and merge them.
void mergeCursors(cursors *cs);

// Apply selection and caret information to the style bytes for a line.
void applyCursors(cursors *cs, int row, chars *styles);

// Carry out a move/mark edit action on cursors.
void moveLeftChar(cursors *cs);
void moveRightChar(cursors *cs);
void moveLeftWord(cursors *cs);
void moveRightWord(cursors *cs);
void moveUpLine(cursors *cs);
void moveDownLine(cursors *cs);
void moveStartLine(cursors *cs);
void moveEndLine(cursors *cs);
void markLeftChar(cursors *cs);
void markRightChar(cursors *cs);
void markLeftWord(cursors *cs);
void markRightWord(cursors *cs);
void markUpLine(cursors *cs);
void markDownLine(cursors *cs);
void markStartLine(cursors *cs);
void markEndLine(cursors *cs);

// Set the cursor to a text position, discarding extra cursors.
void point(cursors *cs, int p);

// Add a cursor and make it current (or delete a cursor at this position).
void addPoint(cursors *cs, int p);

// Extend the current selection to a text position (can't be called select).
void doSelect(cursors *cs, int p);
