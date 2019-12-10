// The Snipe editor is free and open source, see licence.txt.

// For each bracket, store the character and the position in the text.
// TODO: store point, partner, type, open/close/either
struct bracket { int at; char tag; };
typedef struct bracket;

// Bracket matching is done forwards from the start of the text to the cursor,
// and backwards from the end of the text to the cursor. The unmatched open
// brackets before the cursor, and the unmatched close brackets after the
// cursor, are stored. The number of brackets stored is determined roughly by
// the indent level at the cursor position, so is not likely to be large. A gap
// buffer is used, with the gap at the cursor position. That makes it possible
// to make cheap incremental changes. The current cursor position is tracked in
// the at field. The total number of bytes in the text is tracked in max, and
// entries after the gap are stored relative to max.
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

//     if (x == 3) n++;
// _   K _(I_+ _N)_I+ S

// /* x
// < _I
// <

enum tag {
    Or, // Open round bracket '('
    Os, // Open square bracket '['
    Oi, // Open curly-initialiser bracket '|' for '{'
    Oc, // Open curly-block bracket '{'
    Ol, // Open line comment '#', e.g. //
    Om, // Open multiline comment '<', e.g. /*
    Qs, // Single quote '\'', treated as open/close bracket.
    Qd, // Double quote '"', treated as open/close bracket.
    Cr, // Close round bracket ')'
    Cs, // Close square bracket ']'
    Ci, // Close curly-initialiser bracket : for '}', or end
    Cc, // Close curly bracket '}', or end
    Cl, // Close line-comment '\n'
    Cm, // Close multiline comment '>', e.g. */
    Op, // Operator '+'
    Lb, // Label indicator ':'
    Iv, // Invalid '?'
    Ic = 0x80, // In comment (override flag)
    Il = 0x40, // In literal (override flag)
    Um = 0xC0, // Unmatched (override flag)
    TAGS // count the number of tags
};

// Language quirks: (a) \ before \n is ignored.

// Action to perform when comparing an 'old' bracket on top of the stack with a
// 'new' token encountered in the text. Examples relate to forward matching, but
// the actions apply also to backward matching.
enum action {
    I, // ignore     ( x   move past new
    P, // push       ( (   push new, it is an opener
    E, // equal      ( )   pop old, move past new (matched pair)
    G, // greater    ( ]   pop old and mismatch, continue matching new
    L, // less       [ )   mark new as mismatched and move on
    C, // comment    / )   mark new as commented and move on
    Q, // quote      " )   mark new as quoted and move on
    M, // mismatch   " \n  pop old and mismatch, and move on
    N, // nest      /* /*  check for nesting of multiline comments
};

// Working out a range of text to rescan: in language description, work out
// for each tag whether it leaves the scanner in a unique known state.
// Go back to the most recent point just after a token whose tag leaves the
// machine in a known state. Rescan until you reach a token where (a) the tag
// agrees and (b) the machine is left in the known state. Likely problem tags
// are #include for recognising <stdio.h> as a string, and struct/enum/= for
// recognizing open curly brackets which start initialisers.

// Mismatched brackets: only sixteen or so tags count as brackets. Add another
// sixteen 'mismatched bracket' constants. Now we have 26 (A-Z) + 16 (brackets)
// + 16 (mismatched brackets) + 6 (other) making 64.

// Seem to want commented and quoted as overrides, using two bits, leaving 64
// tags available. Maybe can combine commented and quoted as override, with
// nature of override cached with each line. Or maybe, like selections, we can
// have ranges which are post-applied to lines. Or maybe we can 'change' the tag
// and rely on re-bracketing (but then how find the rescan point?)
// Newline + newline-in-comment + newline-in-quotes.

// Lookup table for forward bracket matching. There is a row for each open
// bracket that can appear on the stack.
static char forwardTable[TAGS][TAGS] = {
    //       x  (  [  |  {  // /* '  "  )  ]  :  }  \n */
    [Or] = { I, P, P, P, P  P, P, P, P, E, G, G  G, I, L, }, // (
    [Os] = { I, P, P, P, P, P, P, P, P, L, E, G, I, L, L, }, // [
    [Oc] = { I, P, P, P, P, P, P, P, P, L, L, E, I, L, L, }, // {
    [Ol] = { C, C, C, C, C, C, C, C, C, C, C, C, E, C, C, }, // //
    [Om] = { C, C, C, C, C, C, C, C, C, C, C, C, I, E, C, }, // /*
    [Qs] = { Q, Q, Q, Q, Q, Q, Q, E, Q, Q, Q, Q, M, Q, Q, }, // '
    [Qd] = { Q, Q, Q, Q, Q, Q, Q, Q, E, Q, Q, Q, M, Q, Q, }, // "

}

// Lookup table for backward bracket matching. There is a row for each close
// bracket that can appear on the stack.
static char forwardTable[TAGS][TAGS] = {
    //       x  (  [  |  {  // /* '  "  )  ]  :  }  \n */
    [Qs] = { Q, Q, Q, Q, Q, Q, Q, E, Q, Q, Q,    Q, M, Q, }, // '
    [Qd] = { Q, Q, Q,    Q, Q, Q, Q, E, Q, Q,    Q, M, Q, }, // "
    [Cr] = { I, E, L,    L, G, L, P, P, L, L,    E, ?, L, }, // )
    [Cs] = { ?, C, C,    C, C, C, C, C, C, C,    C, E, C, }, // ]
    [Cc] = { ?, C, C,    C, C, C, C, C, C, C,    C, ?, E, }, // }
    [Cl] = { ?, C, C,    C, C, C, C, C, C, C,    C, ?, C, }, // \n
    [Cm] = { ?, Q, Q,    Q, Q, Q, E, Q, Q, Q,    Q, ?, Q, }, // */
}

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
