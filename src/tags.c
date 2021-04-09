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
// bottom 5 bits are the scanner state after the last token. This works if there
// are <= 64 tags, of which the first <= 32 are for brackets or delimiters.

static char *longNames[TAG_COUNT] = {
    [SKIP]="SKIP", [MORE]="MORE", [GAP]="GAP", [LEFT0]="LEFT0",
    [RIGHT0]="RIGHT0", [LEFT1]="LEFT1", [RIGHT1]="RIGHT1", [LEFT2]="LEFT2",
    [RIGHT2]="RIGHT2", [COMMENT]="COMMENT", [COMMENT0]="COMMENT0",
    [COMMENT1]="COMMENT1", [COMMENT2]="COMMENT2",[COMMENT3]="COMMENT3",
    [COMMENTED]="COMMENTED", [QUOTE]="QUOTE", [DOUBLE]="DOUBLE",
    [TRIPLE]="TRIPLE", [QUOTED]="QUOTED", [NEWLINE]="NEWLINE", [LABEL]="LABEL",
    [ESCAPE]="ESCAPE", [IDENTIFIER]="IDENTIFIER", [TYPE]="TYPE",
    [FUNCTION]="FUNCTION", [PROPERTY]="PROPERTY", [KEYWORD]="KEYWORD",
    [RESERVED]="RESERVED", [VALUE]="VALUE", [OPERATOR]="OPERATOR",
    [SIGN]="SIGN", [BAD]="BAD"
};

static char *shortNames[TAG_COUNT] = {
    [SKIP]="~", [MORE]="-", [GAP]="_", [LEFT0]="(", [RIGHT0]=")", [LEFT1]="[",
    [RIGHT1]="]", [LEFT2]="{", [RIGHT2]="}", [COMMENT]="#", [COMMENT0]="<",
    [COMMENT1]=">", [COMMENT2]="^", [COMMENT3]="$", [COMMENTED]="C",
    [QUOTE]="'", [DOUBLE]="\"", [TRIPLE]="@", [QUOTED]="Q", [NEWLINE]=".",
    [LABEL]=":", [ESCAPE]="E", [IDENTIFIER]="I", [TYPE]="T", [FUNCTION]="F",
    [PROPERTY]="P", [KEYWORD]="K", [RESERVED]="R", [VALUE]="V", [OPERATOR]="O",
    [SIGN]="S", [BAD]="B"
};

tag findTag(char *name) {
    for (tag t = 0; t < TAG_COUNT; t++) {
        if (strcmp(longNames[t], name) == 0) return t;
        if (strcmp(shortNames[t], name) == 0) return t;
    }
    return 255;
}

char shortTagName(tag t) {
    return shortNames[t][0];
}

/*

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
    tag t = 0;
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
    tag t = 0;
    if ((t & 0xE0) == 0xE0) return GAP;
    if ((t & 0xE0) == 0xC0) return BAD;
    if ((t & 0xC0) == 0x80) return COMMENTED;
    if ((t & 0xC0) == 0x40) return QUOTED;
    return t;
}

#ifdef tagsTest

// Check tags are < 64, and ones representing brackets or delimiters are < 32.
static void checkLimits() {
    assert(TAG_COUNT < 64);
    char *bs[] = {"(",")","[","]","{","}","#","<",">","^","$","'","\"","@","."};
    for (int i = 0; i < (sizeof(bs)/sizeof(char*)); i++) {
        assert(findTag(bs[i]) < 32);
    }
    assert(findTag("TAG_COUNT") == 255);
}

int main() {
    checkLimits();
    printf("Tags module OK\n");
    return 0;
}

#endif
