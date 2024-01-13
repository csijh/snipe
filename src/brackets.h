// The Snipe editor is free and open source. See licence.txt.
#include "text.h"

// Brackets are matched forwards from the beginning of the text to the cursor,
// and backwards from the end of the text to the cursor. Remaining surplus
// brackets at the cursor are paired inwards from the ends of the text towards
// the cursor. As the cursor moves, bracket matching is undone and re-done,
// which changes the highlighting of mismatched or unmatched brackets.
typedef struct brackets Brackets;

// Create or free a brackets object.
Brackets *newBrackets();
void freeBrackets(Brackets *bs);

// Just before an edit on the current line, clear it of brackets (backwards from
// the cursor to the start, and forwards from the cursor to the end).
void clearLine(Brackets *bs, Text *t, int lo, int hi);

// Just after an edit on the current line, prepare for re-scanning.
void startLine(Brackets *bs, Text *t, int lo, int hi);

// Ask for the top opener while scanning a line (for bracket-sensitive rules).
int topOpener(Brackets *bs);

// Push an opener on the stack during scanning of a line.
void pushOpener(Brackets *bs, Text *t, int opener);

// Match a closer with the top opener during scanning of a line.
void matchCloser(Brackets *bs, Text *t, int closer);

// Immediately after scanning, ask for the number of outdenters and indenters of
// the line just scanned, providing the indent information for the line.
// (These are the closers that relate to earlier lines, and the openers that
// relate to later lines.)
int outdenters(Brackets *bs);
int indenters(Brackets *bs);

// Return the cursor to the right position after scanning, or track the cursor.
void moveBrackets(Brackets *bs, Text *t, int cursor);
