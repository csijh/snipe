// Snipe editor. Free and open source, see licence.txt.
#include <limits.h>

// Gap buffers for token types, unmatched brackets, matched brackets. The types
// buffer has a byte for each byte of text. The first byte of a token gives its
// type, the remaining bytes have type None. The low part of the unmatched
// buffer is a conventional stack of the indexes of unmatched openers, from the
// start of the text to the current position. The high part is a mirror image
// stack of indexes of unmatched closers, from the end of the text back to the
// current position. The indexes are negative, measured backwards from the end
// of the types buffer. The low part of the matched buffer is a stack of
// matched openers, in the order they were matched, to allow bracket matching
// to be reversed. The high part is a similar stack of matched closers.
struct tokens { byte *types; int *unmatched, *matched; };

// A token index used when an empty stack is popped.
enum { MISSING = INT_MIN };

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
static inline void matchF(Tokens *ts, int t) {
    byte *types = ts->types;
    if (isOpener(types[t])) pushF(ts->unmatched, t);
    else if (isCloser(types[t])) {
        int opener = popF(ts->unmatched);
        if (opener == MISSING) {
            types[t] |= Bad;
        }
        else if (match(types[opener], types[t])) {
            types[opener] &= ~Bad;
            types[t] &= ~Bad;
        }
        else {
            types[opener] |= Bad;
            types[t] |= Bad;
        }
        pushF(ts->matched, opener);
    }
}

// Undo forward bracket matching on token t, assuming pre-allocation.
static inline void unmatchF(Tokens *ts, int t) {
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

// Delete n token bytes before the gap.
void deleteTokens(Tokens *ts, int n) {
    for (int i = t-1; i >= t - n; t--) {
        if (isOpener(ts->types[i]) || isCloser(ts->types[i])) {
            unmatch(ts, i);
        }
    }
}

// Insert n token bytes at the start of the gap, and prepare for scanning,
// pre-allocating the buffers.
void insertTokens(Tokens *ts, int n) {
    int t = length(ts->types);
    ts->types = adjust(ts->types, n);
    for (int i = t; i < t+n; i++) ts->types[i] = None;
    ts->unmatched = ensure(ts->unmatched, n);
    ts->matched = ensure(ts->matched, n);
}

// Check if the top of the unmatched opener stack matches a closer type.
bool matchTop(Tokens *ts, int type) {

}

// Give token at t the given type, and do any appropriate bracket matching.
void setToken(Tokens *ts, int t, int type);

// Set token type (and handle brackets)
// Move cursor.
// Read for display.
