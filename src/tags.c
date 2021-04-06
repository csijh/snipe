// Snipe token and grapheme scanner. Free and open source. See licence.txt.
#include "tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// See text.c
struct tags {
    unsigned char *data;
    int lo, hi, end;
};

tags *newTags(void *table) {
    int n = 1024;
    tags *ts = malloc(sizeof(tags));
    unsigned char *data = malloc(n);
    *ts = (tags) { .lo=0, .hi=n, .end=n, .data=data };
    return ts;
}

// Internally, a tag is flagged using the top two bits, where 0x80 means
// overridden as COMMENTED, 0x40 means QUOTED. If the top bits are 0xC0, then
// the top three bits are used as flags, with 0xC0 meaning a bracket or
// delimiter tag overridden as BAD, and 0xE0 meaning that the tag is GAP and the
// bottom 5 bits are the scanner state after the last token. This only works if
// there are <= 64 tag values, of which the first <= 32 are for brackets or
// delimiters.

static char *longNames[MISS+1] = {
    [G]="GAP", [R]="ROUND0", [r]="ROUND1", [A]="ANGLE0", [a]="ANGLE1",
    [W]="WAVY0", [w]="WAVY1", [C]="COMMENT", [X]="COMMENT0", [x]="COMMENT1",
    [Y]="COMMENT2", [y]="COMMENT3", [c]="COMMENTED", [Q]="QUOTE", [D]="DOUBLE",
    [T]="TRIPLE", [q]="QUOTED", [N]="NEWLINE", [H]="HANDLE", [E]="ESCAPE",
    [I]="ID", [i]="ID1", [F]="FUNCTION", [P]="PROPERTY", [K]="KEY", [k]="KEY1",
    [V]="VALUE", [O]="OPERATOR", [S]="SIGN", [B]="BAD", [J]="JOIN", [M]="MISS",
};

static char shortNames[MISS+1] = {
    [G]='G', [R]='R', [r]='r', [A]='A', [a]='a', [W]='W', [w]='w', [C]='C',
    [X]='X', [x]='x', [Y]='Y', [y]='y', [c]='c', [Q]='Q', [D]='D', [T]='T',
    [q]='q', [N]='N', [H]='H', [E]='E', [I]='I', [i]='i', [F]='F', [P]='P',
    [K]='K', [k]='k', [V]='V', [O]='O', [S]='S', [B]='B', [J]='J', [M]='M'
};
/*

// Characters which visualize tags.
static char tagChar[] = {
    [Byte]='.', [Grapheme]=' ', [Gap]='_', [Operator]='+', [Label]=':',
    [Quote]='\'', [Quotes]='"', [OpenR]='(', [CloseR]=')', [OpenS]='[',
    [CloseS]=']', [OpenC]='{', [CloseC]='}', [OpenI]='%', [CloseI]='|',
    [Comment]='<', [EndComment]='>', [Note]='/', [Newline]='\n', [Invalid]='?',
    [AToken]='A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

// Internally, tags can have two or three override bits added. There must be at
// most 64 tag values, with all brackets and delimiters among the first 32. And
// there must be at most 32 restartable scanner states.
enum override {
    QUOTED = 0x40,      // Inside quotes
    COMMENTED = 0x80,   // Inside a comment
    MISMATCHED = 0xC0,  // Mismatched bracket or delimiter
    GAPPED = 0xE0       // Space character, with scanner state
};

// Do the overriding for a tag, for export.
static inline tag normalise(tag t) {
    if ((t & 0xC0) == 0) return t;
    else if ((t & 0xC0) == QUOTED) return QUOTE;
    else if ((t & 0xC0) == COMMENTED) return COMMENT;
    else if ((t & 0xE0) == MISMATCHED) return BAD;
    else return GAP;
}

char showTag(int tag) {
    switch (tag & 0xC0) {
        case Commented: return tagChar[Gap];
        case Quoted: return tagChar[Gap];
        case Unmatched: return tagChar[Invalid];
        default: return tagChar[tag & 0x3F];
    }
}
*/

void override(tags *ts, int p, tag o) {
    tag t;
    // Can't override GAP.
    if ((t & 0xE0) == 0xE0) return;
    t = t & 0x3F;
    switch (o) {
        case COMMENTED: t = t | 0x80; break;
        case QUOTED:    t = t | 0x40; break;
        case BAD:       t = t | 0xC0; break;
    }
}

tag getTag(tags *ts, int p) {
    tag t;
    if ((t & 0xE0) == 0xE0) return GAP;
    if ((t & 0xE0) == 0xC0) return BAD;
    if ((t & 0xC0) == 0x80) return COMMENTED;
    if ((t & 0xC0) == 0x40) return QUOTED;
    return t;
}

tag findTag(char *name) {
    for (tag t = 0; t <= MISS; t++) {
        if (strcmp(longNames[t], name) == 0) return t;
        if (strlen(name) == 1 && name[0] == shortNames[t]) return t;
    }
    printf("Unknown tag name %s\n", name);
    exit(1);
}

#ifdef tagsTest

// Check tags are < 64, and ones representing brackets or delimiters are < 32.
static void checkLimits() {
    assert(MISS < 64);
    char bs[] = {R,r,A,a,W,w,C,X,x,Y,c,y,Q,D,T,q,N};
    for (int i = 0; i < sizeof(bs); i++) {
        assert(bs[i] < 32);
    }
}

int main() {
    checkLimits();
    printf("Tags module OK\n");
    return 0;
}

#endif
