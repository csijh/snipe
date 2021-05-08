// Snipe language compiler. Free and open source. See licence.txt.
#include "patterns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Memory for one-character strings.
static char singles[128][2];

char *findOrAddPattern(strings *patterns, char *p) {
    for (int i = 0; i < countStrings(patterns); i++) {
        char *s = getString(patterns, i);
        if (strcmp(p, s) == 0) return p;
    }
    addString(patterns, p);
    return p;
}

char *findOrAddPattern1(strings *patterns, char pc) {
    char *s = singles[(int)pc];
    s[0] = pc;
    s[1] = '\0';
    return findOrAddPattern(patterns, s);
}

int findPattern(strings *patterns, char *p) {
    for (int i = 0; i < countStrings(patterns); i++) {
        char *s = getString(patterns, i);
        if (strcmp(p, s) == 0) return i;
    }
    return -1;
}

// Compare two patterns in ASCII order, except suffixes go after longer strings.
static int compare(char *s, char *t) {
    for (int i = 0; ; i++) {
        if (s[i] == '\0' && t[i] == '\0') break;
        if (s[i] == '\0') return 1;
        if (t[i] == '\0') return -1;
        if (s[i] < t[i]) return -1;
        if (s[i] > t[i]) return 1;
    }
    return 0;
}

void sortPatterns(strings *patterns) {
    for (int i = 0; i < countStrings(patterns); i++) {
        char *p = getString(patterns, i);
        int j = i - 1;
        while (j >= 0 && compare(getString(patterns,j), p) > 0) {
            setString(patterns, j+1, getString(patterns, j));
            j--;
        }
        setString(patterns, j+1, p);
    }
}

#ifdef patternsTest

int main() {
    strings *ps = newStrings();
    assert(countStrings(ps) == 0);
    findOrAddPattern(ps, "id");
    assert(countStrings(ps) == 1);
    findOrAddPattern(ps, "id");
    assert(countStrings(ps) == 1);
    findOrAddPattern1(ps, 'x');
    assert(countStrings(ps) == 2);
    assert(strcmp(getString(ps,1), "x") == 0);
    freeStrings(ps);
    return 0;
}

#endif
