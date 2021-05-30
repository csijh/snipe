// Snipe brackets and delimiters. Free and open source. See licence.txt.
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

// NORMAL /* COMMENT
// COMMENT /* COMMENT1
// COMMENT1 /* COMMENT2
// COMMENT2 /* COMMENT3
// COMMENT3 /* COMMENT4
// COMMENT4 /* COMMENT4
// COMMENT4 */ COMMENT3
// COMMENT3 */ COMMENT2
// COMMENT2 */ COMMENT1
// COMMENT1 */ COMMENT
// COMMENT */ NORMAL


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
    XX,  // Not relevant.
+>  PU,  // push next onto openers, then check it against closers
=   MA,  // match with popped opener, push pair onto matched
>   EX,  // excess: mark next as mismatched  (EQUALS G ???)
<   LT,  // less: pop and mismatch opener, repeat
>   GT,  // mismatch next (and push on matched ???)
~   IN,  // incomplete (same as LT, but don't touch NL)
->  SK,  // skip past ordinary token
    CO,  // skip, and override as commented
    QU,  // skip, and override as quoted
};

// Table which compares the most recent opener with the next token, to decide
// what action to take in forward matching. Use M (MISS) for start of text.
int table[COUNT][COUNT] = {
    // At start of text, no openers
    [M][R]=PU, [M][A]=PU, [M][W]=PU, [M][r]=EX, [M][a]=EX, [M][w]=EX,
    [M][Q]=PU, [M][D]=PU, [M][T]=PU, [M][C]=PU, [M][X]=PU, [M][x]=EX,
    [M][Y]=PU, [M][y]=EX, [M][N]=SK, [M][I]=SK,
    // (
    [R][R]=PU, [R][A]=PU, [R][W]=PU, [R][r]=MA, [R][a]=LT, [R][w]=LT,
    [R][Q]=PU, [R][D]=PU, [R][T]=PU, [R][C]=PU, [R][X]=PU, [R][x]=EX,
    [R][Y]=PU, [R][y]=EX, [R][N]=SK, [R][I]=SK,
    // [
    [A][R]=PU, [A][A]=PU, [A][W]=PU, [A][r]=GT, [A][a]=MA, [A][w]=LT,
    [A][Q]=PU, [A][D]=PU, [A][T]=PU, [A][C]=PU, [A][X]=PU, [A][x]=EX,
    [A][Y]=PU, [A][y]=EX, [A][N]=SK, [A][I]=SK,
    // {
    [W][R]=PU, [W][A]=PU, [W][W]=PU, [W][r]=GT, [W][a]=GT, [W][w]=MA,
    [W][Q]=PU, [W][D]=PU, [W][T]=PU, [W][C]=PU, [W][X]=PU, [W][x]=EX,
    [W][Y]=PU, [W][y]=EX, [W][N]=SK, [W][I]=SK,
    // '
    [Q][R]=QU, [Q][A]=QU, [Q][W]=QU, [Q][r]=QU, [Q][a]=QU, [Q][w]=QU,
    [Q][Q]=MA, [Q][D]=QU, [Q][T]=QU, [Q][C]=QU, [Q][X]=QU, [Q][x]=QU,
    [Q][Y]=QU, [Q][y]=QU, [Q][N]=IN, [Q][I]=QU,
    // "
    [D][R]=QU, [D][A]=QU, [D][W]=QU, [D][r]=QU, [D][a]=QU, [D][w]=QU,
    [D][Q]=QU, [D][D]=MA, [D][T]=QU, [D][C]=QU, [D][X]=QU, [D][x]=QU,
    [D][Y]=QU, [D][y]=QU, [D][N]=IN, [D][I]=QU,
    // """
    [T][R]=QU, [T][A]=QU, [T][W]=QU, [T][r]=QU, [T][a]=QU, [T][w]=QU,
    [T][Q]=QU, [T][D]=QU, [T][T]=MA, [T][C]=QU, [T][X]=QU, [T][x]=QU,
    [T][Y]=QU, [T][y]=QU, [T][N]=SK, [T][I]=QU,
    // //
    [C][R]=CO, [C][A]=CO, [C][W]=CO, [C][r]=CO, [C][a]=CO, [C][w]=CO,
    [C][Q]=CO, [C][D]=CO, [C][T]=CO, [C][C]=CO, [C][X]=CO, [C][x]=CO,
    [C][Y]=CO, [C][y]=CO, [C][N]=MA, [C][I]=CO,
    // /*
    [X][R]=CO, [X][A]=CO, [X][W]=CO, [X][r]=CO, [X][a]=CO, [X][w]=CO,
    [X][Q]=CO, [X][D]=CO, [X][T]=CO, [X][C]=CO, [X][X]=GT, [X][x]=MA,
    [X][Y]=CO, [X][y]=CO, [X][N]=SK, [X][I]=CO,
    // {-
    [Y][R]=CO, [Y][A]=CO, [Y][W]=CO, [Y][r]=CO, [Y][a]=CO, [Y][w]=CO,
    [Y][Q]=CO, [Y][D]=CO, [Y][T]=CO, [Y][C]=CO, [Y][X]=CO, [Y][x]=CO,
    [Y][Y]=PU, [Y][y]=MA, [Y][N]=SK, [Y][I]=CO,
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
