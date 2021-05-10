// Snipe language compiler. Free and open source. See licence.txt.
#include "tags.h"
#include "strings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// There are NS symbols and NL letters, making NT possible tag characters.
enum { NS = 32, NL = 26, NT = NS + NL };
static unsigned char tagChars[] =
    "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// An operator says what happens when one bracket or delimiter L meets another
// R. During forward matching, L is on the stack and R is next in the input.
// Then EQ means L and R match, so L is popped. GT means L and R mismatch, with
// R having lower priority and being flagged as an error token. LT means L and R
// mismatch, with R having higher priority, so L is popped and marked as an
// error, and R is considered again. PL means R is pushed on the stack as a new
// opener. IN means L is incomplete, popped, and marked as an error. With
// backward matching, R is on the stack and L is previous in the input, and the
// meanings are similar, e.g. LT means R has higher priority and L is marked as
// mismatched. Often, but not always, triples such as (<] work both forward and
// backward. EG (>> forward but (<> backward.
enum { EQ = '=', GT = '>', LT = '<', PL = '+', IN = '~' };

// An action in a match table consists of an operator and a token type for
// tagging tokens between an opener and closer (or MORE for no tagging).
struct action { char op, type; };
typedef struct action action;

// The bracket,...,closer properties are 0 for false, or a line number for true.
struct tag {
    char ch, in;
    int bracket, delimiter, opener, closer;
};

struct tags {
    int index[128];
    tag *a[NT];
    action forward[NT][NT], backward[NT][NT];
};

static tag *newTag(int in, char ch) {
    struct tag *t = malloc(sizeof(tag));
    t->ch = ch;
    t->in = in;
    t->bracket = t->delimiter = t->opener = t->closer = 0;
    return (tag *) t;
}

tags *newTags() {
    tags *ts = malloc(sizeof(tags));
    for (int i=0; i<128; i++) ts->index[i] = NT;
    for (int i=0; i<NT; i++) ts->index[tagChars[i]] = i;
    for (int i=0; i<NT; i++) ts->a[i] = newTag(i, tagChars[i]);
    for (int i=0; i<NT; i++) {
        for (int j=0; j<NT; j++) {
            ts->forward[i][j] = (action) { ' ', ' ' };
        }
    }
    return ts;
}

void freeTags(tags *ts) {
    for (int i=0; i<NT; i++) free(ts->a[i]);
    free(ts);
}

bool isSymbol(char ch) {
    return ' ' < ch && ch <= '~' && ! isalnum(ch);
}

tag *findTag(tags *ts, int ch) {
    int i = ts->index[ch];
    if (i >= NT) crash("bad tag character %c", ch);
    return ts->a[i];
}

char tagChar(tag *t) { return t->ch; }

bool isBracket(tag *t) { return t->bracket > 0; }
bool isDelimiter(tag *t) { return t->delimiter > 0; }
bool isOpener(tag *t) { return t->opener > 0; }
bool isCloser(tag *t) { return t->closer > 0; }

void setBracket(tag *t, int row) {
    if (t->bracket > 0) return;
    t->bracket = row;
    if (t->in >= NS) {
        crash("a bracket tag must be a symbol (line %d)", row);
    }
    if (t->delimiter > 0) crash(
        "tag is both bracket (line %d) and delimiter (line %d)",
        t->bracket,
        t->delimiter
    );
}

void setDelimiter(tag *t, int row) {
    if (t->delimiter > 0) return;
    t->delimiter = row;
    if (t->in >= NS) {
        crash("a delimiter tag must be a symbol (line %d)", row);
    }
    if (t->bracket > 0) crash(
        "tag is both bracket (line %d) and delimiter (line %d)",
        t->bracket,
        t->delimiter
    );
}

void setOpener(tag *t, int row) {
    if (t->opener) return;
    t->opener = row;
}

void setCloser(tag *t, int row) {
    if (t->closer) return;
    t->closer = row;
}

#ifdef tagsTest

void testTagChars() {
    assert(NS + NL == NT);
    assert(NL == 26);
    for (int i = 0; i < NS; i++) {
        char ch = tagChars[i];
        assert(' ' < ch && ch <= '~');
        assert(! isalnum(ch));
        if (i > 0) assert(ch > tagChars[i-1]);
    }
    for (int i = 0; i < NL; i++) {
        char ch = tagChars[NS + i];
        assert(ch == 'A' + i);
    }
}

int main() {
    testTagChars();
    tags *ts = newTags();
    tag *t1 = findTag(ts, '(');
    tag *t2 = findTag(ts, ')');
    tag *t3 = findTag(ts, '(');
    assert(t1 != t2 && t1 == t3);
    freeTags(ts);
    return 0;
}

#endif
