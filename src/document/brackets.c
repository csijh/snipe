// The Snipe editor is free and open source, see licence.txt.

// For each bracket, store the position in the text and the tag.
struct bracket { int at; char tag; };
typedef struct bracket;

// Bracket matching is done forwards from the start of the text to the cursor,
// and backwards from the end of the text to the cursor. Brackets are stored in
// a gap buffer, with the gap at the current cursor position. The unmatched open
// brackets before the cursor are stored before the gap, and the unmatched close
// brackets after the cursor are stored after the gap, effectively forming two
// stacks. The current cursor position is tracked in the at field. The total
// number of bytes in the text is tracked in max, and positions in entries after
// the gap are relative to max.
struct brackets {
    bracket *data;
    int lo, hi, end;
    int at, max;
};

brackets *newBrackets() {
    brackets *bs = malloc(sizeof(brackets));
    bracket *data = malloc(6 * sizeof(bracket));
    *bs = (brackets) { .lo=0, .hi=6, .end=6, .max=0, .at=0, .data=data };
    return bs;
}

void freeBrackets(brackets *bs) {
    free(bs->data);
    free(bs);
}

static inline top(brackets *bs) {
    return &bs->data[bs->lo - 1];
}

// Each tag is mapped to one of these constants, to extract the bracket info.
enum bracket {
    Nb, // Non-bracket
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
    BRACKETS // count the number of brackets
};

// Action to perform when comparing a bracket on top of the stack with a tag
// encountered in the text. One forward matching example and one backward
// matching example are given for each action.
enum action {
    Move,  // ( x   x )   move past tag
    Push,  // ( (   ) )   push tag as opener
    Match, // ( )   ( )   pop bracket, move past tag
    Pop,   // ( ]   [ )   pop bracket and mismatch, continue matching tag
    Lose,  // [ )   ( ]   mark tag as mismatched and move on
    Note,  // # )   ( #   mark tag as commented and move on
    Quote, // " )   ( "   mark tag as quoted and move on
    Drop,  // " $   $ "   pop bracket and mismatch, and move on
    Nest,  // < <   > >   check for nesting of multiline comments
};

// Lookup table for forward bracket matching. There is an entry [o][b] for each
// open bracket o that can appear on the stack, and each bracket b that can
// appear in the text.
static char forwardTable[][BRACKETS] = {
    [Or][Nb]=Move, [Or][Or]=Push, [Or][Os]=Push, [Or][Oi]=Push, [Or][Oc]=Push,
    [Or][Ol]=Push, [Or][Om]=Push, [Or][Lq]=Push, [Or][Ld]=Push, [Or][Cr]=Match,
    [Or][Cs]=Pop, [Or][Ci]=Pop, [Or][Cc]=Pop, [Or][Cl]=Move, [Or][Cm]=Lose,

    [Os][Nb]=Move, [Os][Or]=Push, [Os][Os]=Push, [Os][Oi]=Push, [Os][Oc]=Push,
    [Os][Ol]=Push, [Os][Om]=Push, [Os][Lq]=Push, [Os][Ld]=Push, [Os][Cr]=Lose,
    [Os][Cs]=Match, [Os][Ci]=Pop, [Os][Cc]=Pop, [Os][Cl]=Move, [Os][Cm]=Lose,

    [Oc][Nb]=Move, [Oc][Or]=Push, [Oc][Os]=Push, [Oc][Oi]=Push, [Oc][Oc]=Push,
    [Oc][Ol]=Push, [Oc][Om]=Push, [Oc][Lq]=Push, [Oc][Ld]=Push, [Oc][Cr]=Lose,
    [Oc][Cs]=Lose, [Oc][Ci]=?, [Oc][Cc]=Match, [Oc][Cl]=Move, [Oc][Cm]=Lose,

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

// Or Nb
static void move() {
    s->point++;
}

//     if (x == 3) n++;
// _   K _(I_+ _N)_I+ S

// /* x
// < _I
// <

// Map tag to bracket...

// Language quirks: (a) \ before \n is ignored.


// TODO: get scanner to issue a tag which encapsulates both the signal character
// and the target state. Working out a range of text to rescan: in language
// description, work out for each tag whether it leaves the scanner in a unique
// known state. Go back to the most recent point just after a token whose tag
// leaves the machine in a known state. Rescan until you reach a token where (a)
// the tag agrees and (b) the machine is left in the known state. Likely problem
// tags are #include for recognising <stdio.h> as a string, and struct/enum/=
// for recognizing open curly brackets which start initialisers. TODO:

// TODO: just override, then re-scan on bracketing change. Seem to want
// commented and quoted as overrides, using two bits, leaving 64 tags available.
// Maybe can combine commented and quoted as override, with nature of override
// cached with each line. Or maybe, like selections, we can have ranges which
// are post-applied to lines. Or maybe we can 'change' the tag and rely on
// re-bracketing (but then how find the rescan point?) Newline +
// newline-in-comment + newline-in-quotes.

// Scan from the top unmatched opener to the current cursor position.
static void matchForward(brackets *bs, int start, int n, char s[n]) {
    bracket *top = top(bs);
    int from = top->at;
    int to = bs->at;
    for (int i = from; i < to; i++) {
        char ch = s[i];


        int at = ...
        bracket *last = ...
        // open v close, last == NULL, last->ch, priority.
        // mark mismatched and then ignore.
    }
}

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

// TODO: in op, store the cursor position as part of the edit.
