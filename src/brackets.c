// Snipe brackets and delimiters. Free and open source. See licence.txt.
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

// Two-letter abbreviations for token types. EN is used for the left and right
// ends of the text. ID is used to represent all token types which are not
// brackets or delimiters.
//enum bracket {
//    EN=GAP, L0=LEFT0, L1=LEFT1, L2=LEFT2, R0=RIGHT0, R1=RIGHT1, R2=RIGHT2,
//    Q0=QUOTE, Q1=QUOTE1, Q2=QUOTE2, C0=COMMENT, C1=COMMENT1, C2=COMMENT2,
//    C3=COMMENT3, C4=COMMENT4, NL=NEWLINE, ID=ID0
//};
enum bracket {
    EN, L0, L1, L2, R0, R1, R2,
    Q0, Q1, Q2, C0, C1, C2,
    C3, C4, NL, ID, COUNT
};

typedef int tag;

bool openers[COUNT] = {
    true, true, true, true, false, false, false,
    true, true, true, true, true, false,
    true, false, false, false
};

// Make sure tags have no override flags when not on any of the stacks.
// (Note indent MAYBE grapheme continuation of preceding NL).
enum action {
    X,  // Not relevant.
    P,  // push next onto openers, then check it against closers
    M,  // match with popped opener, push pair onto matched
    E,  // excess: mark next as mismatched  (EQUALS G ???)
    L,  // less: pop and mismatch opener, repeat
    G,  // mismatch next (and push on matched ???)
    I,  // incomplete (same as L, but don't touch NL)
    S,  // skip past ordinary token
    C,  // skip, and override as commented
    Q,  // skip, and override as quoted
};

// Table which compares the most recent opener with the next token, to decide
// what action to take in forward matching.
int table[COUNT][COUNT] = {
    // No openers
    [EN][L0]=P, [EN][L1]=P, [EN][L2]=P, [EN][R0]=E, [EN][R1]=E, [EN][R2]=E,
    [EN][Q0]=P, [EN][Q1]=P, [EN][Q2]=P, [EN][C0]=P, [EN][C1]=P, [EN][C2]=E,
    [EN][C3]=P, [EN][C4]=E, [EN][NL]=S, [EN][ID]=S,
    // (
    [L0][L0]=P, [L0][L1]=P, [L0][L2]=P, [L0][R0]=M, [L0][R1]=L, [L0][R2]=L,
    [L0][Q0]=P, [L0][Q1]=P, [L0][Q2]=P, [L0][C0]=P, [L0][C1]=P, [L0][C2]=E,
    [L0][C3]=P, [L0][C4]=E, [L0][NL]=S, [L0][ID]=S,
    // [
    [L1][L0]=P, [L1][L1]=P, [L1][L2]=P, [L1][R0]=G, [L1][R1]=M, [L1][R2]=L,
    [L1][Q0]=P, [L1][Q1]=P, [L1][Q2]=P, [L1][C0]=P, [L1][C1]=P, [L1][C2]=E,
    [L1][C3]=P, [L1][C4]=E, [L1][NL]=S, [L1][ID]=S,
    // {
    [L2][L0]=P, [L2][L1]=P, [L2][L2]=P, [L2][R0]=G, [L2][R1]=G, [L2][R2]=M,
    [L2][Q0]=P, [L2][Q1]=P, [L2][Q2]=P, [L2][C0]=P, [L2][C1]=P, [L2][C2]=E,
    [L2][C3]=P, [L2][C4]=E, [L2][NL]=S, [L2][ID]=S,
    // '
    [Q0][L0]=Q, [Q0][L1]=Q, [Q0][L2]=Q, [Q0][R0]=Q, [Q0][R1]=Q, [Q0][R2]=Q,
    [Q0][Q0]=M, [Q0][Q1]=Q, [Q0][Q2]=Q, [Q0][C0]=Q, [Q0][C1]=Q, [Q0][C2]=Q,
    [Q0][C3]=Q, [Q0][C4]=Q, [Q0][NL]=I, [Q0][ID]=Q,
    // "
    [Q1][L0]=Q, [Q1][L1]=Q, [Q1][L2]=Q, [Q1][R0]=Q, [Q1][R1]=Q, [Q1][R2]=Q,
    [Q1][Q0]=Q, [Q1][Q1]=M, [Q1][Q2]=Q, [Q1][C0]=Q, [Q1][C1]=Q, [Q1][C2]=Q,
    [Q1][C3]=Q, [Q1][C4]=Q, [Q1][NL]=I, [Q1][ID]=Q,
    // """
    [Q2][L0]=Q, [Q2][L1]=Q, [Q2][L2]=Q, [Q2][R0]=Q, [Q2][R1]=Q, [Q2][R2]=Q,
    [Q2][Q0]=Q, [Q2][Q1]=Q, [Q2][Q2]=M, [Q2][C0]=Q, [Q2][C1]=Q, [Q2][C2]=Q,
    [Q2][C3]=Q, [Q2][C4]=Q, [Q2][NL]=S, [Q2][ID]=Q,
    // //
    [C0][L0]=C, [C0][L1]=C, [C0][L2]=C, [C0][R0]=C, [C0][R1]=C, [C0][R2]=C,
    [C0][Q0]=C, [C0][Q1]=C, [C0][Q2]=C, [C0][C0]=C, [C0][C1]=C, [C0][C2]=C,
    [C0][C3]=C, [C0][C4]=C, [C0][NL]=M, [C0][ID]=C,
    // /*
    [C1][L0]=C, [C1][L1]=C, [C1][L2]=C, [C1][R0]=C, [C1][R1]=C, [C1][R2]=C,
    [C1][Q0]=C, [C1][Q1]=C, [C1][Q2]=C, [C1][C0]=C, [C1][C1]=G, [C1][C2]=M,
    [C1][C3]=C, [C1][C4]=C, [C1][NL]=S, [C1][ID]=C,
    // {-
    [C3][L0]=C, [C3][L1]=C, [C3][L2]=C, [C3][R0]=C, [C3][R1]=C, [C3][R2]=C,
    [C3][Q0]=C, [C3][Q1]=C, [C3][Q2]=C, [C3][C0]=C, [C3][C1]=C, [C3][C2]=C,
    [C3][C3]=P, [C3][C4]=M, [C3][NL]=S, [C3][ID]=C,
};

// For each opener, check whether it has entries in the table for all tokens.
void checkFull() {
    for (tag t = EN; t < COUNT; t++) {
        if (! openers[t]) continue;
        for (tag u = EN; u < COUNT; u++) {
            if (u == EN) continue;
            if (table[t][u] == X) printf("%d %d\n", t, u);
            assert(table[t][u] != X);
        }
    }
}

// For each token that can be pushed, check it is an opener.
void checkPush() {
    for (tag t = EN; t < COUNT; t++) {
        for (tag u = EN; u < COUNT; u++) {
            if (table[t][u] != P) continue;
            assert(openers[u] && u != EN);
        }
    }
}

// Do consistency checking on the table.  and that some entry pushes it onto the
// opener stack. For each non-opener, check that it is never pushed.
int main(int n, char const *args[]) {
    checkFull();
    checkPush();
    return 0;
}
