// Snipe editor. Free and open source, see licence.txt.
#include "types.h"

// Positions in the text or types are represented as integer indexes.
typedef int index;

// Brackets are matched forwards from the beginning of the text to the cursor,
// and backwards from the end of the text to the cursor. Remaining unpaired
// brackets at the cursor are paired up inwards from the ends towards the
// cursor. As the cursor moves, bracket matching is undone and re-done, which
// changes the highlighting of mismatched or unmatched brackets. All operations
// are done using the types array, without needing the text itself. It is
// assumed that the text and types arrays have a sentinel byte at each end to
// allow surplus unmatched brackets to be treated the same as mismatched
// brackets.
typedef struct brackets Brackets;

// Create or free a brackets object.
Brackets *newBrackets();
void freeBrackets(Brackets *bs);

// Delete brackets from the line between position p and length(ts). An edit
// begins with a deleteBrackets call for the line about to be changed.
void deleteBrackets(Brackets *bs, Type *ts, index p);

// Immediately after an edit, before rescanning of the line, the new line
// contents from p to length(ts) are used for initialisation.
void insertBrackets(Brackets *bs, Type *ts, index p);

// Ask for the top opener while scanning a line (for bracket-sensitive rules).
int topOpener(Brackets *bs);

// Handle an opener during scanning of a line.
void addOpener(Brackets *bs, Type *ts, index opener);

// Handle a closer during scanning of a line.
void addCloser(Brackets *bs, Type *ts, index closer);

// Immediately after scanning, ask for the number of outdenters and indenters of
// the line just scanned, providing the indent information for the line.
int outdenters(Brackets *bs);
int indenters(Brackets *bs);

// Move the cursor to position p immediately after scanning to return the cursor
// to the right place in the line, or between edits to track cursor movement.
// Brackets may be re-highlighted.
void moveBrackets(Brackets *bs, Type *ts, index p);

// =========================
/*
// The sequence of events surrounding an edit to a line is:
//    deleteLine (about to be replaced, p to length types, knowing cursor)
//    undo forward matching from the cursor to the beginning of the line
//    undo backward matching from the cursor to the end of the line
//
//    insertLine (prepare for scanning, lwm for finding indent)
//
//    do forward matching of the line while scanning
//    undo forward matching back to the cursor
//    do backward matching from the end of the line to the cursor
//    repair matching across the cursor

// TODO: Maybe: two (small) stacks and a gap buffer. Pass in types/tags at
// suitable moments.

// Return the most recent opener.
int topOpener(int *openers);

// Add a new opener.
void pushOpener(int *openers, int opener);

// Pair up the top opener, remember it as paired, and return it.
int pairOpener(int *openers);

// Undo a pushOpener.
int popOpener(int *openers);

// Undo the pairing of an opener.
int unpairOpener(int *openers);



// Return the most recent (negative) closer after the cursor.
int topCloser(int *active);

// Add a new (negative) closer after the cursor. Return the (positive) partner
// opener before the cursor, so the pair can be highlighted.
int pushCloser(int *active, int closer);

// Remove the top closer after the cursor. Return the (positive) paired opener
// before the cursor, which now needs to be marked as unmatched.
int popCloser(int *active);

// On pairing two brackets after the cursor, remember the (-ve) closer.
void saveCloser(int *passive, int closer);

// On removing an opener after the cursor, retrieve its (-ve) paired closer,
// which now needs to be pushed on the active stack.
int fetchCloser(int *passive);
*/
