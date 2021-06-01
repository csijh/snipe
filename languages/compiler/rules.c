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
        s[0] = ch;
        if (ch == 0) s[0] = (char) 0x80;
        s[1] = '\0';
        addString(patterns, s);
    }
}

// Find one-character string.
static char *single(int ch) {
    return singles[ch];
}

static int findPattern(strings *ps, char *s) {
    for (int i = 0; i < countStrings(ps); i++) {
        if (strcmp(getString(ps, i), s) == 0) return i;
    }
    return -1;
}

int unescape(char *p, int row) {
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

void escape(char *s) {
    char temp[strlen(s) + 1];
    strcpy(temp, s);
    for (int i = 0; i < strlen(temp); i++) {
        unsigned char ch = temp[i] & 0x7F;
        if ('!' <= ch && ch <= '~') *s++ = ch;
        else s = s + sprintf(s, "\\%d", ch);
    }
    *s = '\0';
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
    for (int i = 0; i < rs->n; i++) freeStrings(rs->a[i]->patterns);
    for (int i = 0; i < rs->n; i++) free((struct rule *)rs->a[i]);
    free(rs->a);
    free(rs);
}

static struct rule *addRule(rules *rs) {
    struct rule *r = malloc(sizeof(rule));
    if (rs->n >= rs->c) {
        rs->c = 2 * rs->c;
        rs->a = realloc(rs->a, rs->c * sizeof(rule *));
    }
    rs->a[rs->n++] = r;
    return r;
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
    else {
        addString(r->patterns, p);
        int n = findPattern(rs->patterns, p);
        if (n < 0) addString(rs->patterns, p);
    }
}

int countRules(rules *rs) {
    return rs->n;
}

extern inline rule *getRule(rules *rs, int i) {
    if (i < 0 || i >= rs->n) return NULL;
    return rs->a[i];
}

// Extract the lookahead flag, if any (as a suffix or separate token).
static bool lookahead(strings *tokens) {
    int n = countStrings(tokens);
    char *last = getString(tokens, n-1);
    if (strcmp(last, "+") == 0) {
        popString(tokens);
        return true;
    }
    int m = strlen(last);
    if (last[m-1] == '+') {
        last[m-1] = '\0';
        return true;
    }
    return false;
}

// Extract the token type, if any.
static char *type(strings *tokens) {
    int n = countStrings(tokens);
    char *last = getString(tokens, n-1);
    if (! isupper(last[0])) return NULL;
    popString(tokens);
    return last;
}

// Read a single rule from the tokens on a line.
static void readRule(rules *rs, int row, strings *tokens) {
    int n = countStrings(tokens);
    if (n == 0) return;
    if (n < 2) crash("rule on line %d too short", row);
    if (! isalpha(getString(tokens, 0)[0])) return;
    struct rule *r = addRule(rs);
    r->row = row;
    r->lookahead = lookahead(tokens);
    r->type = type(tokens);
    n = countStrings(tokens);
    if (n < 2) crash("rule on line %d too short", row);
    char *b = getString(tokens, 0);
    char *t = getString(tokens, n-1);
    if (! islower(b[0])) crash("bad state name %s on line %d", b, row);
    if (! islower(t[0])) crash("bad state name %s on line %d", t, row);
    r->base = b;
    r->target = t;
    r->patterns = newStrings();
    char all[] = "\\0..\\127";
    if (n == 2) readPattern(rs, row, r, all);
    else for (int i = 1; i < n - 1; i++) {
        readPattern(rs, row, r, getString(tokens,i));
    }
}

rules *readRules(char *text) {
    rules *rs = newRules();
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

void printRule(rule *r) {
    printf("%s", r->base);
    for (int i = 0; i < countStrings(r->patterns); i++) {
        char *p = getString(r->patterns, i);
        char temp[10];
        if ((p[0] & 0x7F) <= ' ' || p[0] == 127) {
            sprintf(temp, "\\%d", (p[0] & 0x7F));
            p = temp;
        }
        printf(" %s", p);
    }
    printf(" %s", r->target);
    if (r->lookahead || r->type != NULL) printf(" ");
    if (r->lookahead) printf("-");
    printf("%s\n", r->type == NULL ? "" : r->type);
}

#ifdef rulesTest

void testEscape() {
    char s[100] = "a\200b\37c d\177";
    escape(s);
    assert(strcmp(s, "a\\0b\\31c\\32d\\127") == 0);
}

void testUnescape() {
    char s[] = "ab\\33cd\\32xy";
    int n = unescape(s, 1);
    assert(n == 8);
    assert(strcmp(s, "ab!cd xy") == 0);
}

void showRule(rule *r, char text[]) {
    sprintf(text, "%s", r->base);
    for (int i = 0; i < countStrings(r->patterns); i++) {
        char *p = getString(r->patterns, i);
        char temp[10];
        if ((p[0] & 0x7F) <= ' ' || p[0] == 127) {
            sprintf(temp, "\\%d", (p[0] & 0x7F));
            p = temp;
        }
        sprintf(text + strlen(text), " %s", p);
    }
    sprintf(text + strlen(text), " %s", r->target);
    if (r->lookahead || r->type != NULL) strcat(text, " ");
    strcat(text, r->type == NULL ? "" : r->type);
    if (r->lookahead) strcat(text, "+");
}

void testRule(char const *s, char const *t) {
    rules *rs = newRules();
    strings *tokens = newStrings();
    char text[100], out[500];
    strcpy(text, s);
    splitTokens(1, text, tokens);
    readRule(rs, 1, tokens);
    showRule(getRule(rs, 0), out);
    assert(strcmp(t, out) == 0);
    freeStrings(tokens);
    freeRules(rs);
}

int main(int argc, char const *argv[]) {
    testEscape();
    testUnescape();
    testRule("s + t SIGN",       "s + t SIGN");
    testRule("s + t",            "s + t");
    testRule("s + t +",          "s + t +");
    testRule("s a..c t X",       "s a b c t X");
    testRule("s \\65..\\67 t X", "s A B C t X");
    testRule("s t X",
        "s \\0 \\1 \\2 \\3 \\4 \\5 \\6 \\7 \\8 \\9 \\10 \\11 \\12 \\13 \\14 "
        "\\15 \\16 \\17 \\18 \\19 \\20 \\21 \\22 \\23 \\24 \\25 \\26 \\27 \\28 "
        "\\29 \\30 \\31 \\32 "
        "! \" # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ "
        "A B C D E F G H I J K L M N O P Q R S T U V W X Y Z "
        "[ \\ ] ^ _ ` "
        "a b c d e f g h i j k l m n o p q r s t u v w x y z "
        "{ | } ~ \\127 t X"
    );
    return 0;
}

#endif
