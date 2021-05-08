// Store only long-distance openers and closers.

// To move down a line, find all local bds, pair up, interpret in the light of
// the openers or closers, use to do local overriding, contribute remainder to
// long-distance.

// Tags: start-of-token, continue-char, continue-byte, white (2 bits). Token has
// 'normal' original type. If one-character token, lookup in 128 array. (What if
// two types, context dependent? Treat all but 1 as overrides?) If more, encode
// in second byte (2 bits+6). Overrides: NONE, commented, quoted, bad, -, ~,



// Names with suffix L refer to things to the left of the current position,
// associated with forward matching of brackets and delimiters. Names with
// suffix R refer to things to the right of the current position, associated
// with backward matching.


// TODO:
// *) single table
// *) each tag has an overtag associated with it (or good and bad overtag)
// *) (^> means item on stack wins and its bad GT overtag is used to mismatch
// *) but (^> means GT going right and AB going left
// *) and <v) means AB going right and LT going left
// *) maybe different symbol, not < >, for absorb
// *) with t+u, u is overtagged with its associated (good) overtag

#include <stdio.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Array of tags, and their overriding tags (possibly NONE for no override)

enum { NONE = '-' };

// FIRST reduce tags to a sequence, for compact index, overtags first:
// _.-?*=()[]{}'"#<>. Then have matrix NO x NT to find one-byte code. Invert with
// NC matrix code to pair.

char tags[] = "(()(())))";
char over[] = "---------";
int at = 0;

struct pair { char tag, over; };
typedef struct pair pair;

// Get the pair of tags at position i.
pair getTags(int i) {
    if (i < 0) return (pair) { '-', '-' };
    return (pair) { tags[i], over[i] };
}

// Get the active tag at position i.
char getTag(int i) {
    pair p = getTags(i);
    if (p.over == NONE) return p.tag;
    return p.over;
}

// Set the override tag at position i.
void override(int i, char t) {
    over[i] = t;
}

// -----------------------------------------------------------------------------

// A gap buffer of

// Stacks of unmatched forward openers and backward closers, as a gap buffer.
// Stacks of forward and backward matched pairs, as a gap buffer.
int unmatched[100], matched[100];
int leftUnmatched, rightUnmatched, leftMatched, rightMatched;

// Get the index of last opener.
int lastOpener() {
    if (leftUnmatched == 0) return -1;
    return unmatched[leftUnmatched-1];
}

// Get the index of the last left matched tag.
int lastMatched() {
    if (leftMatched == 0) return -1;
    return matched[leftMatched-1];
}

// Push an opener on the forward stack of openers.
void pushOpener(int i) {
    unmatched[leftUnmatched++] = i;
}

int popOpener() {
    return unmatched[--leftUnmatched];
}

void pushMatchedL(int i) {
    matched[leftMatched++] = i;
}

int popMatchedL() {
    if (leftMatched == 0) return -1;
    return matched[--leftMatched];
}

// -----------------------------------------------------------------------------

struct action { char op, t; };
typedef struct action action;

action forwardTable[128][128];
action backwardTable[128][128];

char *sequence = "-?*=_.()";

char *textTable[] = {
    "    -  (  )  ",
    " -  xx +- >? ",
    " (  <? +- =- ",
    " )  +- xx +- "
};

char *quads[] = { "-+-(", "->?)", "(<?-", "(+-(", "(=-)", ")+--", ")+-)", NULL };

//void setup() {
//
//}

int setup() {
    forwardTable['-']['('] = (action) {'+','-'};
    forwardTable['-'][')'] = (action) {'>','?'};
    forwardTable['(']['('] = (action) {'+','-'};
    forwardTable['('][')'] = (action) {'=','-'};
}

// Get the forward matching action from the last opener and current tag.
action forwardAction() {
    pair l = getTags(lastOpener());
    pair r = getTags(at);
    action a = forwardTable[l.tag][r.tag];
    printf("act %c %c (%c,%c)\n", l.tag, r.tag, a.op, a.t);
    return forwardTable[l.tag][r.tag];
}

// -----------------------------------------------------------------------------

// `...$(a*(b+c))...()...`

// Do a '+' operation, pushing the next tag as an opener.
void plusL(char o) {
    override(at, o);
    pushOpener(at);
}

// Override with o, pop l and push both on 'matchedL'.
int matchL(char o) {
    int l = popOpener();
    pair p = getTags(l);
    override(at, o);
    pushMatchedL(l);
    pushMatchedL(at);
    return l;
}

// Mismatch opener l with closer r, overriding all tags in the range with o.
void mismatchL(char o) {
    int l = matchL(o);
    for (int i = l; i <= at; i++) override(i, o);
}

// Do one step in the forward matching algorithm.
void stepForward() {
    action a = forwardAction();
    printf("step %d %c %c\n", at, a.op, a.t);
    bool repeat = true;
    while (repeat) switch (a.op) {
        case '+': plusL(a.t); repeat = false; break;
        case '=': matchL(a.t); repeat = false; break;
        case '>': override(at, a.t); repeat = false; break;
        case '<': override(popOpener(), a.t); break;
        case '~': mismatchL(a.t); repeat = false; break;
    }
    at++;
}

// Undo one step in the forward matching algorithm.
void undoForward() {
    at--;
    if (lastOpener() == at) popOpener();
    else while (lastMatched() == at) {
        int r = popMatchedL();
        int l = popMatchedL();
        pushOpener(l);
    }
}

void moveRight() {
//    undoBack(); // rept?
    stepForward();
}

int main(int argc, char const *argv[]) {
    setup();
    for (int i = 0; i < 9; i++) stepForward();
    printf("UL %d\n", leftUnmatched);
    printf("ML %d\n", leftMatched);
    printf("O %s\n", over);
    for (int i = 0; i < 8; i++) printf("%d ", matched[i]);
    printf("\n");
    return 0;
}
