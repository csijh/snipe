// The Snipe editor is free and open source, see licence.txt.

// A brackets object keeps track of the brackets in the text, and the (main)
// cursor position, so that bracket matching can be done relative to the cursor.
brackets *newBrackets();
void freeBrackets(brackets *bs) {

// Track insertions, deletions and cursor movements.
void changeBrackets(brackets *bs, op *o);
