// TODO: mmBackward brief forward algorithm to find the matching end tag.
// Snipe language compiler. Free and open source. See licence.txt.
#include "matcher.h"
#include "stacks.h"
#include "tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef unsigned char byte;

// The stacks hold unmatched openers and closers, and matched pairs of tags, for
// the forward and backward matching algorithms. The forward matching table and
// backward matching table each hold an op and an override tag packed into one
// byte in each entry.
struct matcher {
    tags *ts;
    int at;
    stacks *unmatched, *matched;
    byte forward[32][32];
    byte backward[32][32];
    char sequence[32];
    int indexes[128];
};

// Treat SPACE and NEWLINE as override tags, so that 5 bits of data can be stored
// with each (e.g. a scanner state for SPACE and an original indent for NEWLINE).
enum { SPACE = '_', NEWLINE = '.' };

// Opcodes.
enum { EQ, LT, GT, PL, MM };

// Add a tag to the sequence, returning its index.
static int addTag(char sequence[32], char t) {
    for (int i = 0; i < 32; i++) {
        if (sequence[i] == t) return i;
        if (sequence[i] != '\0') continue;
        sequence[i] = t;
        return i;
    }
    crash("too many tags");
    return -1;
}

// Collect override tags from a list of quads.
static void findOver(char *sequence, char *quads[]) {
    for (int i = 0; quads[i] != NULL; i++) {
        char const *quad = quads[i];
        int n = addTag(sequence, quad[2]);
        if (n >= 8) crash("too many override tags");
    }
}

// Collect row/column tags from a list of quads.
static void findTags(char *sequence, char *quads[]) {
    for (int i = 0; quads[i] != NULL; i++) {
        char const *quad = quads[i];
        addTag(sequence, quad[0]);
        addTag(sequence, quad[3]);
    }
}

// Convert an op into a code.
static int opcode(char op) {
    switch (op) {
        case '=': return EQ;
        case '<': return LT;
        case '>': return GT;
        case '+': return PL;
        case '~': return MM;
    }
    crash("unknown op");
    return -1;
}

// Fill in a table entry for each quad in a list.
static void fillTable(matcher *ma, bool isForward, char *quads[]) {
    for (int i = 0; quads[i] != NULL; i++) {
        char *quad = quads[i];
        int row = ma->indexes[quad[0]];
        int col = ma->indexes[quad[3]];
        int b = (opcode(quad[1]) << 5) | ma->indexes[quad[2]];
        if (isForward) ma->forward[row][col] = b;
        else ma->backward[row][col] = b;
    }
}

matcher *newMatcher(char *forward[], char *backward[]) {
    matcher *ma = malloc(sizeof(matcher));
    ma->at = 0;
    for (int i = 0; i < 32; i++) ma->sequence[i] = '\0';
    ma->sequence[0] = SPACE;
    ma->sequence[1] = NEWLINE;
    findOver(ma->sequence, forward);
    findOver(ma->sequence, backward);
    findTags(ma->sequence, forward);
    findTags(ma->sequence, backward);
    ma->ts = newTags(ma->sequence);
    ma->unmatched = newStacks();
    ma->matched = newStacks();
    for (int i = 0; i < 32; i++) ma->indexes[ma->sequence[i]] = i;
    fillTable(ma, true, forward);
    fillTable(ma, false, backward);
    return ma;
}

void freeMatcher(matcher *ma) {
    free(ma->ts);
    free(ma->unmatched);
    free(ma->matched);
    free(ma);
}

// Carry out an EQ operation in the forward direction. Pop the opener, override
// the matching closer, push both on matched stack.
static void eqForward(matcher *ma, char o) {
    int opener = popL(ma->unmatched);
    override(ma->ts, ma->at, o);
    pushL(ma->matched, opener);
    pushL(ma->matched, ma->at);
    ma->at++;
}

// Do a PL operation forwards, pushing the next tag as an opener.
static void plForward(matcher *ma, char o) {
    override(ma->ts, ma->at, o);
    pushL(ma->unmatched, ma->at);
    ma->at++;
}

// Do a GT operation forwards, overriding the next tag.
static void gtForward(matcher *ma, char o) {
    override(ma->ts, ma->at, o);
    ma->at++;
}

// TODO: when undoing ltForward, how undo override on opener?

// Do an LT operation forwards, overriding the opener, pushing the pair as if
// matched, but not moving past the next tag, which needs to be processed again.
static void ltForward(matcher *ma, char o) {
    int opener = popL(ma->unmatched);
    override(ma->ts, opener, o);
    pushL(ma->matched, opener);
    pushL(ma->matched, ma->at);
}

// TODO: how undo all the overrides? Maybe only override opener? Put old override
// at the top of the pushed opener index?

// Do an MM operation forwards, 'matching' the opener with the next tag, but
// overriding all tags in the range with o.
static void mmForward(matcher *ma, char o) {
    int opener = popL(ma->unmatched);
    pushL(ma->matched, opener);
    pushL(ma->matched, ma->at);
    for (int i = opener; i <= ma->at; i++) override(ma->ts, i, o);
    ma->at++;
}

// Do one step in the forward matching algorithm.
void stepForward(matcher *ma) {
    char rowTag = NONE, colTag = NONE;
    int opener = topL(ma->unmatched);
    if (opener >= 0) rowTag = getTag(ma->ts, opener);
    if (ma->at < countTags(ma->ts)) colTag = getTag(ma->ts, ma->at);
    int row = ma->indexes[rowTag], col = ma->indexes[colTag];
    byte b = ma->forward[row][col];
    int op = b >> 5;
    char o = ma->sequence[b & 0x1F];
    bool repeat = true;
    while (repeat) switch (op) {
        case PL: plForward(ma, o); repeat = false; break;
        case EQ: eqForward(ma, o); repeat = false; break;
        case GT: gtForward(ma, o); repeat = false; break;
        case LT: ltForward(ma, o); break;
        case MM: mmForward(ma, o); repeat = false; break;
    }
}

void undoForward() {
    // TODO
}

// Carry out an EQ operation in the backward direction. Pop the closer, override
// the matching opener, push both on matched stack.
static void eqBackward(matcher *ma, char o) {
    ma->at--;
    int closer = popR(ma->unmatched);
    override(ma->ts, ma->at, o);
    pushR(ma->matched, closer);
    pushR(ma->matched, ma->at);
}

// Do a PL operation backwards, pushing the previous tag as a closer.
static void plBackward(matcher *ma, char o) {
    ma->at--;
    override(ma->ts, ma->at, o);
    pushR(ma->unmatched, ma->at);
}

// Do an LT operation backwards, overriding the previous tag.
static void ltBackward(matcher *ma, char o) {
    ma->at--;
    override(ma->ts, ma->at, o);
}

// Do a GT operation backwards, override the closer, push the pair as if
// matched, but don't move past the previous tag, which will be re-processed.
static void gtBackward(matcher *ma, char o) {
    int closer = popR(ma->unmatched);
    override(ma->ts, closer, o);
    pushR(ma->matched, closer);
    pushR(ma->matched, ma->at - 1);
}

// -----------------------------------------------------------------------------

// Do an MM operation backwards, 'matching' the closer with the previous tag,
// but overriding all tags in the range with o.
// TODO: brief forward algorithm to find the matching end tag.
static void mmBackward(matcher *ma, char o) {
    int closer = popR(ma->unmatched);
    pushR(ma->matched, closer);
    pushR(ma->matched, ma->at);
    for (int i = closer; i <= ma->at; i++) override(ma->ts, i, o);
    ma->at++;
}

// Do one step in the backward matching algorithm.
void stepBackward(matcher *ma) {
    char rowTag = NONE, colTag = NONE;
    int closer = topR(ma->unmatched);
    if (closer >= 0) rowTag = getTag(ma->ts, closer);
    if (ma->at < countTags(ma->ts)) colTag = getTag(ma->ts, ma->at);
    int row = ma->indexes[rowTag], col = ma->indexes[colTag];
    byte b = ma->backward[row][col];
    int op = b >> 5;
    char o = ma->sequence[b & 0x1F];
    bool repeat = true;
    while (repeat) switch (op) {
        case PL: plBackward(ma, o); repeat = false; break;
        case EQ: eqBackward(ma, o); repeat = false; break;
        case GT: gtBackward(ma, o); repeat = false; break;
        case LT: ltBackward(ma, o); break;
        case MM: mmBackward(ma, o); repeat = false; break;
    }
}

#ifdef matcherTest

char *quadsF[] = {
    "-+-(", "->?)",
    "(+-(", "(=-)", NULL
};

char *quadsB[] = {
    "(<?-", "(=-)",
    ")+--", ")+-)", NULL
};

int main(int argc, char const *argv[]) {
    matcher *ma = newMatcher(quadsF, quadsB);
    fillTags(ma->ts, "(()(())))");
    for (int i = 0; i < 9; i++) stepForward(ma);
    for (int i = 0; i < 9; i++) printf("%c", getTag(ma->ts, i));
    printf("\n");
    freeMatcher(ma);
    return 0;
}

#endif
