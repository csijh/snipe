// Snipe language compiler. Free and open source. See licence.txt.
#include "rules.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// A rule is normal, or a lookahead (tilde), or a default (no patterns).
enum type { NORMAL, LOOKAHEAD, DEFAULT };

// A rule has a row (line number), a base state, patterns, a target state, and a
// tag. A missing tag is represented as "". The type is one of the above.
struct rule {
    int row;
    char *base, *target;
    strings *patterns;
    char *tag;
    int type;
};
typedef struct rule rule;

struct rules { int c, n; rule **a; char MORE; strings *patterns; };

// Static memory for one-character patterns.
static char singles[128][2];

// Initialise single character strings and add to patterns list.
static void addSingles(strings *patterns) {
    for (int ch = 0; ch < 128; ch++) {
        char *s = singles[ch];
        s[0] = ch;
        s[1] = '\0';
        addString(patterns, s);
    }
}

// Find one-character string.
static char *single(int ch) {
    return singles[ch];
}

rules *newRules(char MORE) {
    rules *rs = malloc(sizeof(rules));
    rule **a = malloc(4 * sizeof(rule *));
    strings *ps = newStrings();
    addSingles(ps);
    *rs = (rules) { .c = 4, .n = 0, .a = a, .MORE = MORE, .patterns = ps };
    return rs;
}

void freeRules(rules *rs) {
    freeStrings(rs->patterns);
    free(rs->a);
    free(rs);
}

static rule *addRule(rules *rs, int row, int tp, char *tg, char *b, char *t) {
    rule *r = malloc(sizeof(rule));
    strings *ps = newStrings();
    *r = (rule) {
        .row = row, .base = b, .target = t, .patterns = ps, .tag = tg,
        .type = tp
    };
    rs->a[rs->n++] = r;
    return r;
}

// Translate escapes in place in the given pattern and return the new length.
// Note \0 may create a null within the string.
static int unescape(char *p) {
    int n = strlen(p);
    for (int i = 0; i < n; i++) {
        if (p[i] != '\\') continue;
        if (! isdigit(p[i+1])) continue;
        int j = i+1;
        bool hex = (p[j] == '0');
        char ch;
        if (hex) {
            ch = strtol(&p[j], NULL, 16);
            while (isxdigit(p[j])) j++;
        }
        else {
            ch = atoi(&p[j]);
            while (isdigit(p[j])) j++;
        }
        p[i] = ch;
        for (int k = j; k <= n; k++) p[i+1+k-j] = p[k];
        n = n - (j - i - 1);
    }
    return n;
}

// Add a pattern to a rule, handling ranges and escapes.
static void readPattern(rules *rs, int row, rule *r, char *p) {
    int n = unescape(p);
    if (n == 4 && p[1] == '.' && p[2] == '.') {
        for (int ch = p[0]; ch <= p[3]; ch++) {
            char *s = single(ch);
            addString(r->patterns, s);
        }
    }
    else addString(r->patterns, p);
}

void readRule(rules *rs, int row, strings *tokens) {
    int n = countStrings(tokens);
    if (n == 0) return;
    if (n == 1) crash("rule on line %d too short", row);
    char *first = getString(tokens, 0);
    char *last = getString(tokens, n-1);
    if (! isalpha(first[0])) return;
    if (! islower(first[0])) crash("bad state name %s on line %d", first, row);
    int type = NORMAL;
    char *tag;
    if (! islower(last[0])) {
        if (n == 2) crash("rule on line %d too short", row);
        tag = last;
        if (tag[0] == '~') {
            type = LOOKAHEAD;
            tag++;
        }
        n--;
        last = getString(tokens, n-1);
    }
    else tag = "";
    if (! islower(last[0])) crash("bad state name %s on line %d", last, row);
    if (n == 2) type = DEFAULT;
    rule *r = addRule(rs, row, type, tag, first, last);
    for (int i = 1; i < n - 1; i++) {
        readPattern(rs, row, r, getString(tokens,i));
    }
}

#ifdef rulesTest

void testUnescape() {
    char s[] = "ab\\33cd\\020xy";
    printf("A\n");
    int n = unescape(s);
    printf("Z\n");
    assert(n == 8);
    assert(strcmp(s, "ab!cd xy") == 0);
}

int main(int argc, char const *argv[]) {
    testUnescape();
    return 0;
}

#endif
