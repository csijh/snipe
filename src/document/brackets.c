// The Snipe editor is free and open source, see licence.txt.
#include "brackets.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>


typedef unsigned char byte;

// For each bracket, store the position in the text and the bracket tag.
struct bracket { int at; char bTag; };
typedef struct bracket bracket;

// Bracket matching is done forwards from the start of the text to the cursor,
// and backwards from the end of the text to the cursor. Brackets are stored in
// a gap buffer, with the gap at the current cursor position. The unmatched open
// brackets before the cursor are stored before the gap, and the unmatched close
// brackets after the cursor are stored after the gap, effectively forming two
// stacks. The current cursor position is tracked in the at field. The total
// number of bytes in the text is tracked in max, and positions in entries after
// the gap are relative to max. The types array and nesting flag are borrowed
// from the scanner (they vary between languages). The tags pointer is a
// temporary pointer to the next tag, at position bs->at in the text.
struct brackets {
    bracket *data;
    int lo, hi, end;
    int at, max;
    char *types;
    bool nesting;
    byte *tags;
};

// Bracket matching is done using tags alone, with needing the original text.
// Each tag is mapped to one of these constants, to extract the bracket info.
enum bracketTag {
    Nb, // Non-bracket
    Ot, // Open text (sentinel)
    Or, // Open round bracket '('
    Os, // Open square bracket '['
    Oc, // Open curly bracket '{'
    Oi, // Open curly initialiser bracket '%' for '{'
    Ol, // Open line comment '#', e.g. //
    Om, // Open multiline comment '<', e.g. /*
    Lq, // Literal single quote '\'', treated as open/close bracket.
    Ld, // Literal double quote '"', treated as open/close bracket.
    Cr, // Close round bracket ')'
    Cs, // Close square bracket ']'
    Cc, // Close curly bracket '}', or end
    Ci, // Close curly-initialiser bracket '~' for '}', or end
    Cl, // Close line-comment '$' for '\n'
    Cm, // Close multiline comment '>', e.g. */
    Ct, // Close text (sentinel)
    BRACKETS // count the number of bracket constants
};

// Actions to perform when comparing a bracket on top of the stack with a byte
// encountered in the text. A forward matching example and a backward matching
// example are given for each action.
enum action {
    Move,  // ( x   x )   move past byte
    Push,  // ( (   ) )   push byte as opener and move on
    Match, // ( )   ( )   pop bracket, move past byte
    Pop,   // ( ]   [ )   pop bracket and mismatch, continue matching byte
    Lose,  // [ )   ( ]   mark byte as mismatched and move on
    Note,  // # )   ( #   mark byte as commented and move on
    Quote, // " )   ( "   mark byte as quoted and move on
    Drop,  // " $   $ "   pop bracket and mismatch, and move on
    Nest,  // < <   > >   check for nesting of multiline comments
};

// Lookup table for forward bracket matching. There is an entry [x][y] for each
// open bracket x that can appear on the stack, and each bracket y that can
// appear next in the text.
static char forwardTable[][BRACKETS] = {
    [Ot][Nb]=Move, [Ot][Or]=Push, [Ot][Os]=Push, [Ot][Oi]=Push, [Ot][Oc]=Push,
    [Ot][Ol]=Push, [Ot][Om]=Push, [Ot][Lq]=Push, [Ot][Ld]=Push, [Ot][Cr]=Lose,
    [Ot][Cs]=Lose, [Ot][Ci]=Lose, [Ot][Cc]=Lose, [Ot][Cl]=Move, [Ot][Cm]=Lose,

    [Or][Nb]=Move, [Or][Or]=Push, [Or][Os]=Push, [Or][Oi]=Push, [Or][Oc]=Push,
    [Or][Ol]=Push, [Or][Om]=Push, [Or][Lq]=Push, [Or][Ld]=Push, [Or][Cr]=Match,
    [Or][Cs]=Pop, [Or][Ci]=Pop, [Or][Cc]=Pop, [Or][Cl]=Move, [Or][Cm]=Lose,

    [Os][Nb]=Move, [Os][Or]=Push, [Os][Os]=Push, [Os][Oi]=Push, [Os][Oc]=Push,
    [Os][Ol]=Push, [Os][Om]=Push, [Os][Lq]=Push, [Os][Ld]=Push, [Os][Cr]=Lose,
    [Os][Cs]=Match, [Os][Ci]=Pop, [Os][Cc]=Pop, [Os][Cl]=Move, [Os][Cm]=Lose,

    [Oc][Nb]=Move, [Oc][Or]=Push, [Oc][Os]=Push, [Oc][Oi]=Push, [Oc][Oc]=Push,
    [Oc][Ol]=Push, [Oc][Om]=Push, [Oc][Lq]=Push, [Oc][Ld]=Push, [Oc][Cr]=Lose,
    [Oc][Cs]=Lose, [Oc][Ci]=Match, [Oc][Cc]=Match, [Oc][Cl]=Move, [Oc][Cm]=Lose,

    [Ol][Nb]=Note, [Ol][Or]=Note, [Ol][Os]=Note, [Ol][Oi]=Note, [Ol][Oc]=Note,
    [Ol][Ol]=Note, [Ol][Om]=Note, [Ol][Lq]=Note, [Ol][Ld]=Note, [Ol][Cr]=Note,
    [Ol][Cs]=Note, [Ol][Ci]=Note, [Ol][Cc]=Note, [Ol][Cl]=Match, [Ol][Cm]=Note,

    [Om][Nb]=Note, [Om][Or]=Note, [Om][Os]=Note, [Om][Oi]=Note, [Om][Oc]=Note,
    [Om][Ol]=Note, [Om][Om]=Note, [Om][Lq]=Note, [Om][Ld]=Note, [Om][Cr]=Note,
    [Om][Cs]=Note, [Om][Ci]=Note, [Om][Cc]=Note, [Om][Cl]=Note, [Om][Cm]=Match,

    [Lq][Nb]=Quote, [Lq][Or]=Quote, [Lq][Os]=Quote, [Lq][Oi]=Quote,
    [Lq][Oc]=Quote, [Lq][Ol]=Quote, [Lq][Om]=Quote, [Lq][Lq]=Match,
    [Lq][Ld]=Quote, [Lq][Cr]=Quote, [Lq][Cs]=Quote, [Lq][Ci]=Quote,
    [Lq][Cc]=Quote, [Lq][Cl]=Quote, [Lq][Cm]=Match,

    [Ld][Nb]=Quote, [Ld][Or]=Quote, [Ld][Os]=Quote, [Ld][Oi]=Quote,
    [Ld][Oc]=Quote, [Ld][Ol]=Quote, [Ld][Om]=Quote, [Ld][Lq]=Quote,
    [Ld][Ld]=Match, [Ld][Cr]=Quote, [Ld][Cs]=Quote, [Ld][Ci]=Quote,
    [Ld][Cc]=Quote, [Ld][Cl]=Quote, [Ld][Cm]=Match,
};

// Create a new brackets object, with a sentinel bracket at each end.
brackets *newBrackets() {
    brackets *bs = malloc(sizeof(brackets));
    bracket *data = malloc(6 * sizeof(bracket));
    *bs = (brackets) { .lo=1, .hi=5, .end=6, .max=0, .at=0, .data=data };
    bs->data[0] = (bracket) { .at = 0, .bTag = Ot };
    bs->data[bs->hi] = (bracket) { .at = 0, .bTag = Ct };
    return bs;
}

void freeBrackets(brackets *bs) {
    free(bs->data);
    free(bs);
}

static void resize(brackets *bs) {
    int size = bs->end;
    size = size * 3 / 2;
    bs->data = realloc(bs->data, size * sizeof(bracket));
    int n = bs->end - bs->hi;
    memmove(&bs->data[size - n], &bs->data[bs->hi], n * sizeof(bracket));
    bs->hi = size - n;
    bs->end = size;
}

// Get the type associated with a tag.
static inline int getType(brackets *bs, byte tag) {
    return bs->types[tag];
}

// Convert a scanner tag into a bracket tag.
static int getBracketTag(brackets *bs, byte tag) {
    switch (getType(bs, tag)) {
        case '\'': return Lq;
        case '"': return Ld;
        case '(': return Or;
        case ')': return Cr;
        case '[': return Os;
        case ']': return Cs;
        case '{': return Oc;
        case '}': return Cc;
        case '%': return Oi;
        case '!': return Ci;
        case '<': return Om;
        case '>': return Cm;
        case '#': return Ol;
        case '$': return Cl;
        default: return Nb;
    }
}

// Get the top open bracket on the forward stack.
static inline bracket *topForward(brackets *bs) {
    return &bs->data[bs->lo - 1];
}

// Mark a tag as a mismatched bracket.
static inline void mismatch(brackets *bs, int at) {
    // TODO
}

// Mark a tag as commented.
static inline void commented(brackets *bs, int at) {
    // TODO
}

// Mark a tag as quoted.
static inline void quoted(brackets *bs, int at) {
    // TODO
}

// Actions to perform during forward matching when comparing a bracket on top of
// the stack with the next byte in the text.

// Eg  ( x  move past a byte during forward scanning.
static inline void moveForward(brackets *bs) {
    bs->at++;
    bs->tags++;
}

// Eg  ( (  push byte as opener and move on
static inline void pushForward(brackets *bs) {
    if (bs->lo >= bs->hi) resize(bs);
    bs->data[bs->lo++] = (bracket) { .at = bs->at, .bTag = bs->tags[0] };
    moveForward(bs);
}

// Eg  ( )  pop bracket, move past byte
static inline void matchForward(brackets *bs) {
    bs->lo--;
    moveForward(bs);
}

// Eg  ( ]  pop bracket and mismatch it, don't move on
static inline void popForward(brackets *bs) {
    bs->lo--;
    mismatch(bs, bs->data[bs->lo].at);
}

// Eg  [ )  mark byte as mismatched and move on
static inline void loseForward(brackets *bs) {
    mismatch(bs, bs->at);
    moveForward(bs);
}

// Eg  # )  mark byte as commented and move on
static inline void noteForward(brackets *bs) {
    commented(bs, bs->at);
    moveForward(bs);
}

// Eg  " )  mark tag as quoted and move on
static inline void quoteForward(brackets *bs) {
    quoted(bs, bs->at);
    moveForward(bs);
}

// Eg  " $  pop bracket and mismatch it, and move on
static inline void dropForward(brackets *bs) {
    popForward(bs);
    moveForward(bs);
}

// Eg  < <  check for nesting of multiline comments
static inline void nestForward(brackets *bs) {
    if (bs->nesting) pushForward(bs);
    else noteForward(bs);
}

// Match brackets forward through the given tags
static void matchingForward(brackets *bs, int n, byte tags[n]) {
    bs->tags = tags;
    for (int i = 0; i < n; i++) {
        int openBracket = getBracketTag(bs, topForward(bs)->bTag);
        int nextBracket = getBracketTag(bs, tags[i]);
        int action = forwardTable[openBracket][nextBracket];
        switch (action) {
            case Move: moveForward(bs); break;
            case Push: pushForward(bs); break;
            case Match: matchForward(bs); break;
            case Pop: popForward(bs); break;
            case Lose: loseForward(bs); break;
            case Note: noteForward(bs); break;
            case Quote: quoteForward(bs); break;
            case Drop: dropForward(bs); break;
            case Nest: nestForward(bs); break;
            default: break;
        }
    }
}

// Forward bracket matching tests. The first of each pair of strings is a
// sequence of token types. The second shows which brackets remain on the stack
// and which types have been altered after matching.
static char *test[] = {
    "K(I)$",
    "     "
};

/*
void changeBrackets(brackets *bs, op *o) {
    // range TODO fix, and check same as style.
    int start = atOp(o);
    int end = start + lengthOp(o);
    int cursor = atOp(o);
    // Update max. (Does it affect start, end?)
    // Delete entries between start and end.
    // Ensure range covers cursor.
    // Match from start to cursor, adding to gap start.
    // Match back from end to cursor, adding to gap end.
    // Apply style changes to all the brackets stored.
    // Make sure previously marked brackets, not stored, remain marked.
}
*/
