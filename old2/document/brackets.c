// The Snipe editor is free and open source, see licence.txt.
#include "brackets.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef unsigned char byte;

// Unmatched bracket positions and types are stored in a gap buffer, with the
// gap at the current cursor position. The unmatched open brackets before the
// cursor are stored before the gap, and the unmatched close brackets after the
// cursor are stored after the gap, effectively forming two stacks. The current
// cursor position is tracked in the at field. The total number of bytes in the
// text is tracked in max, and positions in entries after the gap are relative
// to max. The map array maps tags to their bracket values, and the nesting flag
// says whether multiline comments can nest. They vary between languages. The
// tags array provides direct but ephemeral access to the tags, either from 0 to
// at for forward matching or at to max for backward matching.
struct brackets {
    int *positions;
    byte *types;
    int lo, hi, end;
    int at, max;
    char map[256];
    bool nesting;
    byte tags;
};

// Bracket matching is done using tags alone, without needing the original text.
// Each tag is mapped to one of these constants, to extract the bracket info.
enum bracket {
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

// Actions to perform when comparing a bracket on top of the stack with the tag
// of a byte encountered in the text. A forward matching example and a backward
// matching example are given for each action.
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
// open bracket x that can appear on the stack, and each bracket type y that can
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
    int n = 24;
    brackets *bs = malloc(sizeof(brackets));
    int *positions = malloc(n * sizeof(int));
    byte *types = malloc(n * sizeof(byte));
    *bs = (brackets) { .lo=1, .hi=n-1, .end=n, .at=0, .max=0 };
    bs->positions = positions;
    bs->positions[0] = -1;
    bs->positions[n-1] = 0;
    bs->types = types;
    bs->types[0] = Ot;
    bs->types[n-1] = Ct;
    return bs;
}

void freeBrackets(brackets *bs) {
    free(bs->positions);
    free(bs->types);
    free(bs);
}

static void resize(brackets *bs) {
    int size = bs->end;
    size = size * 3 / 2;
    bs->positions = realloc(bs->positions, size * sizeof(int));
    bs->types = realloc(bs->types, size * sizeof(byte));
    int n = bs->end - bs->hi;
    memmove(&bs->positions[size - n], &bs->positions[bs->hi], n * sizeof(int));
    memmove(&bs->types[size - n], &bs->types[bs->hi], n * sizeof(byte));
    bs->hi = size - n;
    bs->end = size;
}

// Get the bracket type associated with a tag.
static inline int getBracket(brackets *bs, byte tag) {
    return bs->map[tag];
}

// Convert a token type character into a bracket.
static int getBracketTag(brackets *bs, char type) {
    switch (type) {
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
    return &bs->types[bs->lo - 1];
}

// Mark a tag as a mismatched bracket.
static inline void mismatch(brackets *bs, int at, byte *tags) {
    tags[at] = 0xC0 | tags[at];
}

// Mark a tag as commented.
static inline void commented(brackets *bs, int at, byte *tags) {
    tags[at] = 0x80 | tags[at];
}

// Mark a tag as quoted.
static inline void quoted(brackets *bs, int at) {
    tags[at] = 0x40 | tags[at];
}

// Actions to perform during forward matching when comparing a bracket on top of
// the stack with the next byte in the text.

static inline void pushForward(brackets *bs, int at, byte bracket) {
    if (bs->lo >= bs->hi) resize(bs);
    bs->positions[bs->lo] = at;
    bs->types[bs->lo++] = bracket;
}

static inline void popForward(brackets *bs) {
    bs->lo--;
}

static inline void moveForward(brackets *bs) {
    // TODO
}

// COULD BE JUST pop; return true.
// Eg  ( )  pop bracket, move past byte
static inline void matchForward(brackets *bs) {
    popForward(bs);
    moveForward(bs);
}

// COULD BE JUST pop; mismatch(p); return false.
// Eg  ( ]  pop bracket and mismatch it, don't move on
static inline void popForward(brackets *bs) {
    bs->lo--;
    mismatch(bs, bs->data[bs->lo].at);
}

// COULD BE JUST mismatch(tag); return true.
// Eg  [ )  mark byte as mismatched and move on
static inline void loseForward(brackets *bs) {
    mismatch(bs, bs->at);
    moveForward(bs);
}

// COULD BE JUST commented; return true.
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

// NEEDS: positions of brackets, access to tag array to get info about stacked
// brackets, next tag, return true/false to move on.

// Move past the given tags in the forward bracket matching algorithm, as the
// result of an insertion or rightwards move.
static void doForward(brackets *bs, int at, int n, byte tags[n]) {
    byte open = bs->types[bs->lo - 1];
    for (int i = 0; i < n; i++) {
        byte next = bs->map[tags[i]];
        int action = forwardTable[open][next];
        switch (action) {
            case Move: at++; break;
            case Push: pushForward(bs, at, next); at++; break;
            case Match: popForward(bs); at++; break;
            case Pop: popForward(bs); i--; break;
            case Lose: loseForward(bs); break;
            case Note: noteForward(bs); break;
            case Quote: quoteForward(bs); break;
            case Drop: dropForward(bs); break;
            case Nest: nestForward(bs); break;
            default: break;
        }
    }
}

// Undo one tag in forward bracket matching algorithm.
// TODO: because of repair of tags, haven't got the old ones to undo from. So,
// probably only C and E apply.
// undoForward(brackets *bs, int at, byte tag) { ... }
// A: IF at is position of top bracket (== tag) then just pop.
// B: IF tag is open bracket, MUST be A
// C: IF top bracket is < or " then unmark tag (or do some rescanning?)
// D: IF tag is Nb just move back
// E: have closer, now must re-bracket forwards from top bracket

static void undoForward(brackets *bs, int to) {
    while (top-pos >= to) popForward(bs);
    if (top is < or " or ') move to to;
    else {
        move to top;
        redo to to;
    }
}

// Translation table: types[] and targets[]. For newly added tags (i >= index)
// types[i] holds override type and targets[i] holds previous tag value. COULD
// recognise via override types '-' for comment, '~' for literal, '!' for
// mismatched tag.

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
