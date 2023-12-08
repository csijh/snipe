// Snipe editor. Free and open source, see licence.txt.

// Maintain brackets. Calls to deleteBrackets, insertBrackets, moveBrackets are
// used to track gap-based changes in the text and types arrays. Changes are
// made to the types array to reflect matching or mismatching brackets.
struct brackets;
typedef struct brackets Brackets;

// Create a new brackets object, initially empty.
Brackets *newBrackets();

// Delete brackets in the n bytes before the gap. (Called to track a deletion of
// n bytes from the types array.)
void deleteBrackets(Brackets *ts, int n);

// Insert n bytes at the start of the gap. (Called to track an insertion.)
void insertBrackets(Brackets *ts, int n);

// Move the cursor to p. (Called to track movement in text/tokens)
void moveBrackets(Brackets *ts, byte *types, int p);

//
void addToken(Brackets *ts, byte *types, int p);

// Record an open bracket which has just been added at position p.
void addOpener(Brackets *ts, byte *types, int p);

// Record a close bracket which has just been added at position p.
void addCloser(Brackets *ts, byte *types, int p);

// Add a close bracket only if it matches the top opener, returning success.
bool tryCloser(Brackets *ts, byte *types, int p);


// TODO: Store closers as well as openers in matched. Sentinels?
// Solves unquotes. Allows state in newline.
// TODO: Track matching of unmatched brackets.
