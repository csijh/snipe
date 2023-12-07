// Snipe editor. Free and open source, see licence.txt.
#include <limits.h>

// TODO: who extends the types array, and how does the brackets object keep up?

// Brackets for a document are held in two gap buffers, one for unmatched and
// one for matched brackets. The brackets object also has access to the array
// of types for the document during one scan, for which the types array is
// pre-allocated. The low part of the unmatched buffer is a conventional stack
// of the indexes of unmatched openers, from the start of the text to the
// current position. The high part is a mirror image stack of indexes of
// unmatched closers, from the end of the text back to the current position.
// The indexes in the high stack are negative, measured backwards from the end
// of the types buffer. The low part of the matched buffer is a stack of
// matched openers, in the order they were matched, to allow bracket matching
// to be reversed. The high part similarly holds matched closers.
struct brackets { int *unmatched, *matched; byte *types; };

// A token index used when an empty stack is popped.
enum { MISSING = INT_MIN };

// Mark two brackets as matched or mismatched, given that one may be missing.
static inline mark(byte *types, int open, int close) {
    if (open == MISSING) types[close] |= Bad;
    else if (close == MISSING) types[open] |= Bad;
    else if (bracketMatch(types[open], types[close])) {
        types[open] &= ~Bad;
        types[close] &= ~Bad;
    else {
        types[open] |= Bad;
        types[close] |= Bad;
    }
}

// Push a token index onto a forward stack, assuming pre-allocation.
static inline void pushF(int *stack, int t) {
    int n = length(stack);
    adjust(stack, +1);
    stack[n] = t;
}

// Pop a token index (or MISSING) from a forward stack.
static inline int popF(int *stack) {
    int n = length(stack);
    if (n == 0) return MISSING;
    adjust(stack, -1);
    return stack[n-1];
}

// Push a (negative) token index onto a backward stack, assuming pre-allocation.
static inline void pushB(int *stack, int t) {
    int n = high(stack);
    rehigh(stack, -1);
    stack[n-1] = t;
}

// Pop a (negative) token index (or MISSING) from a backward stack.
static inline int popB(int *stack) {
    int m = max(stack);
    int h = high(stack);
    if (h == m) return MISSING;
    rehigh(stack, +1);
    return stack[h-1];
}

// Do forward bracket matching on token t, assuming pre-allocation.
static inline void matchF(Brackets *ts, int t) {
    byte *types = ts->types;
    int type = types[t];
    if (isOpener(type)) pushF(ts->unmatched, t);
    else if (isCloser(type)) {
        int opener = popF(ts->unmatched);
        mark(ts->types, opener, t);
        pushF(ts->matched, opener);
    }
}

// Undo forward bracket matching on token t, assuming pre-allocation.
static inline void unmatchF(Brackets *ts, int t) {
    byte *types = ts->types;
    types[t] &= ~Bad;
    if (isOpener(types[t])) popF(ts->unmatched, t);
    else if (isCloser(types[t])) {
        int opener = popF(ts->matched);
        if (opener != MISSING) {
            types[opener] &= ~Bad;
            pushF(ts->unmatched, opener);
        }
    }
}

// Do backward bracket matching on token t, assuming pre-allocation.
static inline void matchB(Brackets *ts, int t) {
    byte *types = ts->types;
    int type = types[t];
    if (isCloser(type)) pushB(ts->unmatched, t);
    else if (isOpener(type)) {
        int closer = popB(ts->unmatched);
        mark(ts->types, type, closer);
        pushB(ts->matched, closer);
    }
}

// Undo backward bracket matching on token t, assuming pre-allocation.
static inline void unmatchB(Brackets *ts, int t) {
    byte *types = ts->types;
    types[t] &= ~Bad;
    if (isCloser(types[t])) popB(ts->unmatched, t);
    else if (isOpener(types[t])) {
        int closer = popB(ts->matched);
        if (closer != MISSING) {
            types[closer] &= ~Bad;
            pushB(ts->unmatched, closer);
        }
    }
}

// Delete n token bytes before the gap.
void deleteBrackets(Brackets *ts, int n) {
    for (int i = t-1; i >= t - n; t--) {
        if (isOpener(ts->types[i]) || isCloser(ts->types[i])) {
            unmatchF(ts, i);
        }
    }
}

// Insert n token bytes at the start of the gap, and prepare for scanning,
// pre-allocating the stack buffers.
void insertBrackets(Brackets *ts, int n) {
    int t = length(ts->types);
    ts->types = adjust(ts->types, n);
    for (int i = t; i < t+n; i++) ts->types[i] = None;
    ts->unmatched = ensure(ts->unmatched, n);
    ts->matched = ensure(ts->matched, n);
}

// Check if the top of the unmatched opener stack matches a closer type.
bool matchTop(Brackets *ts, int type) {

}

// Give token at t the given type, and do any appropriate bracket matching.
void setBracket(Brackets *ts, int t, int type);

// TODO: Set token type? (and handle brackets)
// TODO: Move cursor.

//
