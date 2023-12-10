// Snipe editor. Free and open source, see licence.txt.

// Brackets are matched forwards from the beginning of the text to the cursor,
// and backwards from the end of the text to the cursor. As the cursor moves,
// brackets are unmatched and re-matched accordingly, which enables
// highlighting to be changed.

// Brackets are stored in two integer gap buffers, as index positions in the
// text, so that they are independent of re-allocations of the text buffer.
// Positions after the cursor are stored as negative indexes, relative to the
// end of the text, so that they are immune to insertions and deletions at the
// cursor. It is assumed that the text has a sentinel byte at each end, at
// positions 0 and -1. These allow unmatched (surplus) brackets to be treated
// the same as mismatched brackets.

// The low end of the active gap buffer holds openers (positions of open
// brackets) before the cursor, which haven't been matched up to that point, as
// with conventional bracket matching. The high end of the active buffer holds
// closers after the cursor, in mirror image fashion. The openers are paired up
// with the closers, inwards from the ends towards the cursor.

// The low end of the passive gap buffer holds openers which have been paired up
// before the cursor, in the order that the partner close brackets were
// encountered. This allows bracket matching to be undone easily. The high end
// similarly holds closers which have been paired up after the cursor.

// Return the most recent opener.
int topOpener(int *active);

// Add a new opener before the cursor. Return the (negative) paired closer
// after the cursor, so the pair can be highlighted as matched or mismatched.
int pushOpener(int *active, int opener);

// Remove the top opener before the cursor. Return the (negative) paired closer
// after the cursor, which now needs to be marked as unmatched.
int popOpener(int *active);

// On pairing two brackets, remember the opener.
void saveOpener(int *passive, int opener);

// On removing a closer, retrieve its paired opener, which now needs to be
// pushed on the active stack.
int fetchOpener(int *passive);

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
