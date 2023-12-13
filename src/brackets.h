// Snipe editor. Free and open source, see licence.txt.
#include "types.h"

// Brackets are matched forwards from the beginning of the text to the cursor,
// and backwards from the end of the text to the cursor. Remaining unpaired
// brackets at the cursor are paired up inwards from the ends towards the
// cursor. As the cursor moves, bracket matching is undone and re-done, which
// changes the highlighting of mismatched or unmatched brackets. Positions in
// the text or types are represented as integer indexes. All operations are
// done using the types array, without needing the text itself. It is assumed
// that the text and types arrays have a sentinel byte at each end to allow
// surplus unmatched brackets to be treated the same as mismatched brackets.
typedef struct brackets Brackets;

// Create or free a brackets object.
Brackets *newBrackets();
void freeBrackets(Brackets *bs);

// Delete brackets from the line between position p and length(ts). An edit
// begins with a clearLine call for the line about to be changed.
void clearLine(Brackets *bs, Type *ts, int p);

// Immediately after an edit, before rescanning of the line, the new line
// contents from p to length(ts) are used for initialisation.
void startLine(Brackets *bs, Type *ts, int p);

// Ask for the top opener while scanning a line (for bracket-sensitive rules).
int topOpener(Brackets *bs);

// Handle an opener during scanning of a line.
void pushOpener(Brackets *bs, Type *ts, int opener);

// Handle a closer during scanning of a line.
void matchCloser(Brackets *bs, Type *ts, int closer);

// Immediately after scanning, ask for the number of outdenters and indenters of
// the line just scanned, providing the indent information for the line.
int outdenters(Brackets *bs);
int indenters(Brackets *bs);

// Move the cursor to position p immediately after scanning to return the cursor
// to the right place in the line, or between edits to track cursor movement.
// Brackets may be re-highlighted.
void moveBrackets(Brackets *bs, Type *ts, int p);
