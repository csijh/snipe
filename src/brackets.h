// Snipe editor. Free and open source, see licence.txt.
#include "types.h"

// Brackets are matched forwards from the beginning of the text to the cursor,
// and backwards from the end of the text to the cursor. Remaining unpaired
// brackets at the cursor are paired up inwards from the ends towards the
// cursor. As the cursor moves, bracket matching is undone and re-done, which
// changes the highlighting of mismatched or unmatched brackets. All operations
// are done using the types array, without needing the text itself. Positions
// are represented as gap buffer indexes (i.e. negative after the gap). Surplus
// unmatched brackets are represented as the position MISSING, so they can be
// treated the same as mismatched brackets.
typedef struct brackets Brackets;

// Create or free a brackets object.
Brackets *newBrackets();
void freeBrackets(Brackets *bs);

// Start processing a line that has been edited, between positions lo and hi.
// Brackets should have been cleared from the line before the edit.
void startLine(Brackets *bs, Type *ts, int lo, int hi);

// Ask for the top opener while scanning a line (for bracket-sensitive rules).
int topOpener(Brackets *bs);

// Push an opener on the stack during scanning of a line.
void pushOpener(Brackets *bs, Type *ts, int opener);

// Match a closer with the top opener during scanning of a line.
void matchCloser(Brackets *bs, Type *ts, int closer);

// Immediately after scanning, ask for the number of outdenters and indenters of
// the line just scanned, providing the indent information for the line.
// (These are the closers that relate to earlier lines, and the openers that
// relate to later lines.)
int outdenters(Brackets *bs);
int indenters(Brackets *bs);

// Re-match brackets forwards between lo and to (when scanning is not needed).
void matchForward(Brackets *bs, Type *ts, int lo, int hi);

// Undo forward matching between lo and hi.
void clearForward(Brackets *bs, Type *ts, int lo, int hi);

// (Re-)match brackets backwards between lo and hi (both negative).
void matchForward(Brackets *bs, Type *ts, int lo, int hi);

// Undo backward matching between lo and hi (both negative).
void clearBackward(Brackets *bs, Type *ts, int lo, int hi);
