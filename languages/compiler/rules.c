// Snipe language compiler. Free and open source. See licence.txt.
#include "rules.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

struct rules { int c, n; rule **a; strings *patterns; };

// Static memory for one-character patterns.
static char singles[128][2];

// Initialise single character strings and add to patterns list.
static void addSingles(strings *patterns) {
    for (int ch = 0; ch < 128; ch++) {
        char *s = singles[ch];
        if (ch == 0) ch = 0x80;
        s[0] = ch;
        s[1] = '\0';
        addString(patterns, s);
    }
}

// Find one-character string.
static char *single(int ch) {
    return singles[ch];
}

static rules *newRules() {
    rules *rs = malloc(sizeof(rules));
    rule **a = malloc(4 * sizeof(rule *));
    strings *ps = newStrings();
    addSingles(ps);
    *rs = (rules) { .c = 4, .n = 0, .a = a, .patterns = ps };
    return rs;
}

void freeRules(rules *rs) {
    freeStrings(rs->patterns);
    free(rs->a);
    free(rs);
}

static rule *addRule(rules *rs, int row, char *b, char *t, bool l, char *tag) {
    struct rule *r = malloc(sizeof(rule));
    strings *ps = newStrings();
    *r = (struct rule) {
        .row = row, .base = b, .target = t, .patterns = ps, .tag = tag,
        .lookahead = l
    };
    rs->a[rs->n++] = r;
    return r;
}


// Translate escapes in place in the given pattern and return the new length.
// Note \0 may create a null within the string.
static int unescape(char *p, int row) {
    int n = strlen(p);
    for (int i = 0; i < n; i++) {
        if (p[i] != '\\') continue;
        if (! isdigit(p[i+1])) continue;
        if (p[i+1] == '0' && isdigit(p[i+2])) {
            crash("bad escape on line %d", row);
        }
        int j = i+1;
        int ch = atoi(&p[j]);
        while (isdigit(p[j])) j++;
        if (ch < 0 || ch > 127) crash("character out of range on line %d", row);
        if (ch == 0) ch = 0x80;
        p[i] = ch;
        for (int k = j; k <= n; k++) p[i+1+k-j] = p[k];
        n = n - (j - i - 1);
    }
    return n;
}

// Add a pattern to a rule, handling ranges and escapes.
static void readPattern(rules *rs, int row, rule *r, char *p) {
    int n = unescape(p, row);
    if (n == 4 && p[1] == '.' && p[2] == '.') {
        for (int ch = (p[0] & 0x7F); ch <= (p[3] & 0x7F); ch++) {
            char *s = single(ch);
            addString(r->patterns, s);
        }
    }
    else addString(r->patterns, p);
}

int countRules(rules *rs) {
    return rs->n;
}

extern inline rule *getRule(rules *rs, int i) {
    if (i < 0 || i >= rs->n) return NULL;
    return rs->a[i];
}

// Read a single rule from the tokens on a line.
static void readRule(rules *rs, int row, strings *tokens) {
    int n = countStrings(tokens);
    if (n == 0) return;
    if (n == 1) crash("rule on line %d too short", row);
    char *first = getString(tokens, 0);
    char *last = getString(tokens, n-1);
    if (! isalpha(first[0])) return;
    if (! islower(first[0])) crash("bad state name %s on line %d", first, row);
    bool lookahead = false;
    char *tag;
    if (! islower(last[0])) {
        if (n == 2) crash("rule on line %d too short", row);
        tag = last;
        if (tag[0] == '~') {
            lookahead = true;
            tag++;
        }
        if (tag[0] == '~') crash("symbol ~ used as token type on line %d", row);
        n--;
        last = getString(tokens, n-1);
    }
    else tag = "-";
    if (strlen(tag) == 0) tag = "-";
    if (isdigit(tag[0]) || islower(tag[0])) crash("bad tag on line %d", row);
    if (! islower(last[0])) crash("bad state name %s on line %d", last, row);
    if (n == 2) lookahead = true;
    rule *r = addRule(rs, row, first, last, lookahead, tag);
    if (n == 2) readPattern(rs, row, r, "\0..127");
    for (int i = 1; i < n - 1; i++) {
        readPattern(rs, row, r, getString(tokens,i));
    }
}

rules *readRules(char const *path) {
    rules *rs = newRules();
    char *text = readFile(path, false);
    strings *lines = newStrings();
    splitLines(text, lines);
    strings *tokens = newStrings();
    for (int i = 0; i < countStrings(lines); i++) {
        clearStrings(tokens);
        splitTokens(i+1, getString(lines, i), tokens);
        readRule(rs, i+1, tokens);
    }
    freeStrings(tokens);
    freeStrings(lines);
    return rs;
}

void checkTags(rules *rs) {
    strings *names = newStrings();
    for (int i = 0; i < rs->n; i++) {
        rule *r = rs->a[i];
        if (strlen(r->tag) == 1) continue;
        for (int j = 0; j < countStrings(names); j++) {
            char *s = getString(names, j);
            if (s[0] != r->tag[0]) continue;
            if (strcmp(s, r->tag) == 0) continue;
            crash("two tags start with %c (line %d)", s[0], r->row);
        }
        addString(names, r->tag);
    }
    freeStrings(names);
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

static void sortPatterns(strings *patterns) {
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

strings *getPatterns(rules *rs) {
    sortPatterns(rs->patterns);
    return rs->patterns;
}

#ifdef rulesTest

void testUnescape() {
    char s[] = "ab\\33cd\\32xy";
    int n = unescape(s, 1);
    assert(n == 8);
    assert(strcmp(s, "ab!cd xy") == 0);
}

int main(int argc, char const *argv[]) {
    testUnescape();
    return 0;
}

#endif
