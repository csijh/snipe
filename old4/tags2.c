// Snipe language compiler. Free and open source. See licence.txt.
#include "tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tags are held in a gap buffer, with an original and overriding tag packed
// into each byte. The sequence field holds the tags relevant to a language. The
// indexes field maps tag characters to their indexes.
typedef unsigned char byte;
struct tags {
    int lo, hi, end;
    byte *data;
    char sequence[32+1];
    int indexes[128];
};

// Create a new tags object, given the sequence of tags relevant to a language.
tags *newTags(char const *tagSequence) {
    tags *ts = malloc(sizeof(tags));
    int n = 16;
    byte *bs = malloc(n);
    *ts = (tags) { .lo = 0, .hi = n, .end = n, .data = bs };
    strcpy(ts->sequence, tagSequence);
    for (int i = 0; i < strlen(tagSequence); i++) {
        ts->indexes[tagSequence[i]] = i;
    }
    return ts;
}

// Unpack the two tags from a byte.
static inline pair unpack(tags *ts, byte b) {
    pair p;
    p.tag = ts->sequence[b & 0x1F];
    p.over = ts->sequence[b >> 5];
    return p;
}

// Get the pair of tags at position i, with a notional NONE at either end.
pair getTags(tags *ts, int i) {
    if (i < 0) return (pair) { NONE, NONE };
    if (i >= ts->lo) i = i + ts->hi - ts->lo;
    if (i < ts->end) return unpack(ts, ts->data[i]);
    return (pair) { NONE, NONE };
}

// Get the active tag at position i, taking the override into account.
char getTag(tags *ts, int i) {
    pair p = getTags(ts, i);
    if (p.over == NONE) return p.tag;
    return p.over;
}

// Set the tag at position i, with no override.
void setTag(tags *ts, int i, char t) {
    if (i >= ts->lo) i = i + ts->hi - ts->lo;
    ts->data[i] = ts->indexes[t];
}

// Set or reset the override tag at position i.
void override(tags *ts, int i, char o) {
    if (i >= ts->lo) i = i + ts->hi - ts->lo;
    ts->data[i] = (ts->indexes[o] << 5) | (ts->data[i] & 0x1F);
}

#ifdef tagsTest

int main(int argc, char const *argv[]) {
    return 0;
}

#endif
