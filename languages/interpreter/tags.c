// Snipe language compiler. Free and open source. See licence.txt.
#include "tags.h"
#include "stacks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Tags are held in a byte array bs of length n. An original and overriding tag
// are packed into each byte. The sequence field holds the tags relevant to a
// language. The indexes field maps tag characters to their indexes in the
// sequence.
enum { SIZE = 100 };
typedef unsigned char byte;
struct tags {
    int n;
    byte bs[SIZE];
    char sequence[32];
    int indexes[128];
};

// Pack two tags, original t and override o, into a byte.
static inline byte pack(tags *ts, char t, char o) {
    return (ts->indexes[o] << 5) | ts->indexes[t];
}

// Unpack the two tags from a byte.
static inline pair unpack(tags *ts, byte b) {
    pair p;
    p.tag = ts->sequence[b & 0x1F];
    p.over = ts->sequence[b >> 5];
    return p;
}

tags *newTags(char *tagChars) {
    tags *ts = malloc(sizeof(tags));
    ts->n = 0;
    int n = strlen(tagChars);
    if (n > 32) crash("too many tags");
    for (int i = 0; i < n; i++) ts->sequence[i] = tagChars[i];
    for (int i = 0; i < n; i++) ts->indexes[ts->sequence[i]] = i;
    return ts;
}

void fillTags(tags *ts, char *s) {
    ts->n = strlen(s);
    if (ts->n > SIZE) crash("line too long");
    for (int i = 0; i < ts->n; i++) {
        ts->bs[i] = pack(ts, s[i], NONE);
//        printf("C %c %d %c\n", s[i], ts->indexes[s[i]], getTag(ts,i));
    }
    pair p = getTags(ts, 0);
}

int countTags(tags *ts) {
    return ts->n;
}

pair getTags(tags *ts, int i) {
    if (i < 0 || i >= ts->n) crash("index into tags out of range");
    byte b = ts->bs[i];
    return unpack(ts, ts->bs[i]);
}

char getTag(tags *ts, int i) {
    pair p = getTags(ts, i);
    if (p.over == NONE) return p.tag;
    else return p.over;
}

void override(tags *ts, int i, char o) {
    if (i < 0 || i >= ts->n) crash("index into tags out of range");
    ts->bs[i] = (ts->indexes[o] << 5) | (ts->bs[i] & 0x1F);
}

#ifdef tagsTest

int main(int argc, char const *argv[]) {
    tags *ts = newTags("-_.?()");
    assert(ts->sequence[4] == '(');
    assert(ts->indexes['('] == 4);
    free(ts);
    return 0;
}

#endif
