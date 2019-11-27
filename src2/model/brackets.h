// Snipe bracket matcher. Free and open source. See licence.txt.

// A brackets object keeps track of the brackets in the text, and does
// incremental bracket matching.
brackets *newBrackets();
void freeBrackets(brackets *bs) {

// Track insertions, deletions and cursor movements.
void changeBrackets(brackets *bs, op *o);
