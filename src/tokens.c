// Snipe editor. Free and open source, see licence.txt.
#include "tokens.h"
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>

char *typeNames[64] = {
    [None]="None", [Alternative]="Alternative", [Block]="Block",
    [Comment]="Comment", [Declaration]="Declaration", [Error]="Error",
    [Function]="Function", [Group]="Group", [H]="H", [Identifier]="Identifier",
    [Jot]="Jot", [Keyword]="Keyword", [L]="L", [Mark]="Mark", [Note]="Note",
    [Operator]="Operator", [Property]="Property", [Quote]="Quote",
    [Round]="Round", [Square]="Square", [Tag]="Tag", [Unary]="Unary",
    [Value]="Value", [White]="White", [X]="X", [Y]="Y", [Z]="Z",

    [QuoteB]="QuoteB", [Quote2B]="Quote2B", [CommentB]="CommentB",
    [Comment2B]="Comment2B", [TagB]="TagB", [RoundB]="RoundB",
    [Round2B]="Round2B", [SquareB]="SquareB",[Square2B]="Square2B",
    [GroupB]="GroupB", [Group2B]="Group2B", [BlockB]="BlockB",
    [Block2B]="Block2B",

    [QuoteE]="QuoteE", [Quote2E]="Quote2E", [CommentE]="CommentE",
    [Comment2E]="Comment2E", [TagE]="TagE", [RoundE]="RoundE",
    [Round2E]="Round2E", [SquareE]="SquareE",[Square2E]="Square2E",
    [GroupE]="GroupE", [Group2E]="Group2E", [BlockE]="BlockE",
    [Block2E]="Block2E"
};

char *typeName(int type) {
    return typeNames[type];
}

byte displayType(int type) {
    if ((type & Bad) != 0) return Error;
    if (type < FirstB) return type;
    return typeNames[type][0] - 'A' + 1;
}

char visualType(int type) {
    bool bad = (type & Bad) != 0;
    type = type & ~Bad;
    if (type == None) return '-';
    if (type == White) return ' ';
    type = typeNames[type][0];
    if (bad) type = tolower(type);
    return type;
}

bool isPrefix(int type) {
    type = type & ~Bad;
    switch (type) {
    case BlockB: case Block2B: case BlockE: case Block2E:
    case CommentB: case Comment: case CommentE: case Comment2B: case Comment2E:
    case GroupB: case Group2B: case QuoteB: case Quote2B: case Quote: case Mark:
    case Note: case Operator: case RoundB: case Round2B:
    case SquareB: case Square2B: case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

bool isPostfix(int type) {
    type = type & ~Bad;
    switch (type) {
    case BlockB: case Block2B: case GroupE: case Group2E: case Mark:
    case Operator: case RoundE: case Round2E: case SquareE: case Square2E:
    case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

// Token information is stored in three gap buffers. The types buffer has a byte
// for each byte of source text, with a sentinel byte at each end (as does the
// source text), and with the gap at the current cursor position. The first
// byte for each token holds its type, and the remaining bytes hold the type
// None. Brackets are stored in the unmatched and matched buffers. The low part
// of the unmatched buffer is a conventional stack of the indexes of unmatched
// openers, between the start of the text and the cursor. The high part is a
// mirror image stack of indexes of unmatched closers, from the end of the text
// back to the cursor. The indexes in the high stack are negative, measured
// backwards from the end of the text. The low part of the matched buffer is a
// stack of matched openers up to the cursor, in the order they were matched,
// to allow bracket matching to be reversed easily. The high part similarly
// holds matched closers after the cursor. While adding tokens to a line,
// remember the start position of the line and use it to count the number of
// outdenting brackets (matched with openers on previous lines) and the number
// of indenting brackets (openers still open at the end of the line).
struct tokens { byte *types; int *unmatched, *matched; int line, outs, ins; };

// Create new tokens object, with sentinels at either end of the types buffer.
Tokens *newTokens() {
    Tokens *ts = malloc(sizeof(Tokens));
    ts->types = newArray(sizeof(byte));
    ts->unmatched = newArray(sizeof(int));
    ts->matched = newArray(sizeof(int));
    ts->types = ensure(ts->types, 2);
    adjust(ts->types, 1);
    rehigh(ts->types, -1);
    ts->types[0] = White;
    ts->types[high(ts->types)] = White;
    return ts;
}

// Set all the new types to None. Make sure the other buffers have enough
// capacity for the scanning of the new text.
void insertLine(Tokens *ts, int n) {
    int start = length(ts->types);
    ts->types = adjust(ts->types, n);
    for (int i = start; i < start+n; i++) ts->types[i] = None;
    ts->unmatched = ensure(ts->unmatched, n);
    ts->matched = ensure(ts->matched, n);
}

// Push a bracket onto a forward stack, assuming capacity already ensured.
static inline void pushF(int *stack, int t) {
    int n = length(stack);
    adjust(stack, +1);
    stack[n] = t;
}

// Pop a bracket from a forward stack (or sentinel).
static inline int popF(int *stack) {
    int n = length(stack);
    if (n == 0) return 0;
    adjust(stack, -1);
    return stack[n-1];
}

// Find the top of a forward stack (or the start sentinel).
static inline int topF(int *stack) {
    int n = length(stack);
    if (n == 0) return 0;
    return stack[n-1];
}

// Check whether two brackets match.
static inline bool bracketMatch(byte open, byte close) {
    return close == open + FirstE - FirstB;
}

// Mark two brackets as matched or mismatched.
static inline void mark(Tokens *ts, int opener, int closer) {
    byte *types = ts->types;
    if (bracketMatch(types[opener], types[closer])) {
        types[opener] &= ~Bad;
        types[closer] &= ~Bad;
    }
    else {
        types[opener] |= Bad;
        types[closer] |= Bad;
    }
}

// TODO: track start of current line, #closers, #openers.

// TODO: do this as we go along, or fix at end of line?
// Push an opener onto the unmatched stack, and mark it and its
// partner on the high stack (or sentonel) as matched or mismatched.
static inline void pushOpener(Tokens *ts, int opener) {
    int n = length(ts->unmatched);
    int h = high(ts->unmatched);
    int m = max(ts->unmatched);
    int closer = m - 1;
    if (h <= m - n - 1) closer = ts->unmatched[m - n - 1];
    pushF(ts->unmatched, opener);
    mark(ts, opener, closer);
}

void deleteLine(Tokens *ts, int n) {
    int p = length(ts->types);
    for (int i = p-1; i >= p-n; i--) {
        if (isOpener(types[t])) popF(ts->unmatched, t);
        else if (isCloser(types[t])) {
            int opener = popF(ts->matched);
            // TODO: check now or later
            pushOpener(ts->unmatched, opener);
        }
    }
}

// =================================================================

// Track a cursor movement to position p. Brackets may be re-highlighted.
void moveTokens(Tokens *ts, int p) {
// unmatch front, match back.
}

// Add a token at position p. Update brackets as appropriate.
void addToken(Tokens *ts, int p, byte type) {
    ts->types[p] = type;
    if (isOpener(type)) pushF(ts->unmatched, p);
    else if (isCloser(type)) {
        int opener = popF(ts->unmatched);
        mark(ts->types, opener, p);
        pushF(ts->matched, opener);
    }
}

// Add a closer, only if it matches the top opener, returning success.
bool tryToken(Tokens *ts, int p, byte type) {
    return true;
}

// Read n token bytes from position p into the given byte array, returning
// the possibly reallocated array.
byte *readTokens(Tokens *ts, int p, int n, byte *bs) {
    return bs;
}

// ---------- Testing ----------------------------------------------------------
#ifdef tokensTest

int main() {
    for (int t = 0; t < LastE; t++) {
        assert(typeNames[t] != NULL);
        if (t == None) {
            assert(displayType(t) == t);
            assert(visualType(t) == '-');
        }
        else if (t == White) {
            assert(displayType(t) == t);
            assert(visualType(t) == ' ');
        }
        else {
            assert(1 <= displayType(t) && displayType(t) <= 26);
            assert(displayType(t|Bad) == Error);
            assert('A' <= visualType(t) && visualType(t) <= 'Z');
            assert('a' <= visualType(t|Bad) && visualType(t|Bad) <= 'z');
        }
    }
    printf("Tokens module OK\n");
}

#endif
