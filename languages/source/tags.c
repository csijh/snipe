// Snipe language compiler. Free and open source. See licence.txt.
#include "tags.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

struct tag moreTag = {
    .bracket = false, .delimiter = false, .opener = false, .closer = false,
    .ch = '-', .name1 = "-", .name = "-"
};

tag *more = &moreTag, *skip, *gap, *newline;

// The 32 allowable ASCII symbols (excluding controls, space, letters, digits).
static char symbols[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

bool isSymbol(char *s) {
    if (strlen(s) != 1) return false;
    for (int i = 0; i < strlen(symbols); i++) {
        if (s[0] == symbols[i]) return true;
    }
    return false;
}

// Check if a string is described by a tag.
static bool eqTag(char *s, tag *t) {
    if (s[0] != t->name[0]) return false;
    if (strcmp(s, t->name) == 0) return true;
    if (strlen(s) != 1) crash("tags %s and %s are not consistent", s, t->name);
    return true;
}

static tag *newTag(char *s) {
    if (! isupper(s[0]) && ! isSymbol(s)) crash("bad tag %s", s);
    int n = strlen(s);
    struct tag *t = malloc(sizeof(tag) + n + 1);
    strcpy(t->name, s);
    t->ch = s[0];
    t->name1[0] = s[0];
    t->name1[1] = '\0';
    t->bracket = t->delimiter = t->opener = t->closer = false;
    return (tag *) t;
}

// Find an existing tag or create a newly allocated one.
tag *findTag(tag ***tsp, char *s) {
    tag **ts = *tsp;
    int i;
    for (i = 0; ts[i] != NULL; i++) if (eqTag(s, ts[i])) return ts[i];
    tag *t = newTag(s);
    addTag(tsp, t);
    return t;
}

void freeTag(tag *t) { free((struct tag *) t); }

#ifdef tagsTest

int main(int argc, char const *argv[]) {
    tag **tags = newTags();
    tag *t1 = findTag(&tags, "(");
    tag *t2 = findTag(&tags, ")");
    tag *t3 = findTag(&tags, "(");
    assert(t1 != t2 && t1 == t3);
    freeTag(t1);
    freeTag(t2);
    free(tags);
    return 0;
}

#endif
