// Snipe language compiler. Free and open source. See licence.txt.

// A matcher carries out bracket and delimiter matching.
struct matcher;
typedef struct matcher matcher;

// Create a new matcher, given forward and backward matching tables for a
// language, each expressed as a NULL terminated list of quads. Each quad is a
// string of four characters. The first and last of the four represent a row and
// column in the table, the second is one of = < > + ~ representing an
// operation, and the third is the associated override tag. Between the two
// tables, there should be at most 8 tags used as overrides (including NONE,
// SPACE, NEWLINE) and at most 32 in total.
matcher *newMatcher(char *forward[], char *backward[]);

// Free up a matcher, and its tag array and stacks.
void freeMatcher(matcher *ma);



/*
//

// The forward matching table and backward
// matching table each hold an op and an override tag packed into one byte in
// each entry. The unmatched stacks hold unmatched openers and unmatched
// closers. The matched stacks hold matched pairs of tags.
*/

/*
// Structure holding everything to do with tags and matching. Tags are held in
// an array bs, with an original and overriding tag packed into each byte. The
// sequence field holds the tags relevant to a language. The indexes field maps
// tag characters to their indexes in the sequence. The forward matching table
// and backward matching table each hold an op and an override tag packed into
// one byte in each entry. The unmatched stacks hold unmatched openers and
// unmatched closers. The matched stacks hold matched pairs of tags.
typedef unsigned char byte;
struct tags {
    int n, at;
    byte bs[100];
    char sequence[32];
    int indexes[128];
    byte forward[32][32];
    byte backward[32][32];
    stacks *unmatched, *matched;
};

// An original tag and corresponding override tag.
struct pair { char tag, over; };
typedef struct pair pair;

// Treat SPACE and NEWLINE as override tags, so that 5 bits of data can be stored
// with each (a scanner state for SPACE and an original indent for NEWLINE).
enum { SPACE = '_', NEWLINE = '.' };

// Opcodes.
enum { EQ, LT, GT, PL, MM };

// Add a tag to the sequence, returning its index.
static int addTag(tags *ts, char t) {
    for (int i = 0; i < 32; i++) {
        if (ts->sequence[i] == t) return i;
        if (ts->sequence[i] != '\0') continue;
        ts->sequence[i] = t;
        return i;
    }
    crash("too many tags");
    return -1;
}

// Collect override tags from a list of quads.
static void findOver(tags *ts, char *quads[]) {
    for (int i = 0; quads[i] != NULL; i++) {
        char const *quad = quads[i];
        int n = addTag(ts, quad[2]);
        if (n >= 8) crash("too many override tags");
    }
}

// Collect row/column tags from a list of quads.
static void findTags(tags *ts, char *quads[]) {
    for (int i = 0; quads[i] != NULL; i++) {
        char const *quad = quads[i];
        addTag(ts, quad[0]);
        addTag(ts, quad[3]);
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
static void fillTable(tags *ts, bool isForward, char *quads[]) {
    for (int i = 0; quads[i] != NULL; i++) {
        char *quad = quads[i];
        int row = ts->indexes[quad[0]];
        int col = ts->indexes[quad[3]];
        int b = (opcode(quad[1]) << 5) | ts->indexes[quad[2]];
        if (isForward) ts->forward[row][col] = b;
        else ts->backward[row][col] = b;
    }
}

tags *newTags(char *forward[], char *backward[]) {
    tags *ts = malloc(sizeof(tags));
    ts->n = ts->at = 0;
    for (int i = 0; i < 32; i++) ts->sequence[i] = '\0';
    ts->sequence[0] = SPACE;
    ts->sequence[1] = NEWLINE;
    findOver(ts, forward);
    findOver(ts, backward);
    findTags(ts, forward);
    findTags(ts, backward);
    for (int i = 0; i < 32; i++) ts->indexes[ts->sequence[i]] = i;
    fillTable(ts, true, forward);
    fillTable(ts, false, backward);
    initStacks(&ts->unmatched);
    initStacks(&ts->matched);
    return ts;
}

// Unpack the two tags from a byte.
static inline pair unpack(tags *ts, byte b) {
    pair p;
    p.tag = ts->sequence[b & 0x1F];
    p.over = ts->sequence[b >> 5];
    return p;
}

pair getTags(tags *ts, int i) {
    if (i < 0 || i >= ts->n) return (pair) { NONE, NONE };
    return unpack(ts, ts->bs[i]);
}

char getTag(tags *ts, int i) {
    pair p = getTags(ts, i);
    if (p.over == NONE) return p.tag;
    else return p.over;
}

void fillTags(tags *ts, char *s) {
    ts->n = strlen(s);
    for (int i = 0; i < ts->n; i++) ts->bs[i] = ts->indexes[s[i]];
}

// Add or replace an override tag at position i, given its index.
void override(tags *ts, int i, int oi) {
    ts->bs[i] = (oi << 5) | (ts->bs[i] & 0x1F);
}


// Carry out an EQ operation in the forward direction. Pop an opener, override
// the matching closer, push bot on matched stack.
void eqForward(tags *ts, int oi) {
    int opener = popL(&ts->unmatched);
    override(ts, ts->at, oi);
    pushL(&ts->matched, opener);
    pushL(&ts->matched, ts->at);
    ts->at++;
}

#ifdef tagsTest

char *quads[] = {
    "-+-(", "->?)",
    "(<?-", "(+-(", "(=-)",
    ")+--", ")+-)", NULL
};

int main(int argc, char const *argv[]) {
    tags *ts = newTags(quads, quads);
    assert(strcmp(ts->sequence, "_.-?()") == 0);
    assert(ts->indexes['('] == 4);
    assert(ts->forward[4][4] == ((PL<<5)|2));
    free(ts);
    return 0;
}

#endif
*/
