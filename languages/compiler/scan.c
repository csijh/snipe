#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// Report error in the style of printf and exit.
void crash(char const *fmt, ...) {
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

// Read file and normalise. Check for controls or Unicode.
char *readFile(char const *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("can't read file %s", path);
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("can't find size of file %s", path);
    int capacity = size + 2;
    char *s = malloc(capacity);
    int n = fread(s, 1, size, file);
    if (n != size) crash("can't read file %s", path);
    if (n > 0 && s[n - 1] != '\n') s[n++] = '\n';
    s[n] = '\0';
    for (int i = 0; i < n; i++) {
        if (s[i] == '\n') continue;
        if (s[i] == '\t' || s[i] == '\r') s[i] = ' ';
        else if (s[i] < ' ' || s[i] > 127) {
            crash("file %s contains illegal characters", path);
        }
    }
    fclose(file);
    return s;
}

// ---------- Tokens -----------------------------------------------------------

// A list of token strings.
typedef struct { int length, max; char **a; } tokens;

// Create a token list.
tokens *newT() {
    tokens *ts = malloc(sizeof(tokens));
    *ts = (tokens) { .length = 0, .max = 8, .a = malloc(8 * sizeof(char *)) };
    return ts;
}

int lengthT(tokens *ts) { return ts->length; }
char *getT(tokens *ts, int i) { return ts->a[i]; }

// Change the length of a list by n items (n may be negative).
void resizeT(tokens *ts, int n) {
    ts->length += n;
    if (ts->length <= ts->max) return;
    while (ts->length > ts->max) ts->max = ts->max * 3 / 2;
    ts->a = realloc(ts->a, ts->max * sizeof(char *));
}

// Free a token list. Optionally free the strings.
void freeT(tokens *ts, bool strings) {
    if (strings) { for (int i = 0; i < ts->length; i++) free(getT(ts,i)); }
    free(ts->a);
    free(ts);
}

// Find a token, or allocate a new one.
char *findOrAlloc(tokens *ts, char *s) {
    for (int i = 0; i < lengthT(ts); i++) {
        if (strcmp(s, getT(ts,i)) == 0) return getT(ts,i);
    }
    char *copy = malloc(strlen(s) + 1);
    strcpy(copy, s);
    resizeT(ts, 1);
    ts->a[lengthT(ts)-1] = copy;
    return copy;
}

// Find a token, or add a new one (using pointer comparison for unique tokens).
void findOrAdd(tokens *ts, char *s) {
    for (int i = 0; i < lengthT(ts); i++) {
        if (s == getT(ts,i)) return;
    }
    resizeT(ts, 1);
    ts->a[lengthT(ts)-1] = s;
}

// --------- States ------------------------------------------------------------

typedef struct { char *name; } state;
typedef struct { int length, max; state **a; } states;

// Create a state list.
states *newS() {
    states *ss = malloc(sizeof(states));
    *ss = (states) { .length = 0, .max = 8, .a = malloc(8 * sizeof(state *)) };
    return ss;
}

int lengthS(states *ss) { return ss->length; }
state *getS(states *ss, int i) { return ss->a[i]; }

void resizeS(states *ss, int n) {
    ss->length += n;
    if (ss->length <= ss->max) return;
    while (ss->length > ss->max) ss->max = ss->max * 3 / 2;
    ss->a = realloc(ss->a, ss->max * sizeof(char *));
}

void freeS(states *ss) {
    for (int i = 0; i < ss->length; i++) {
        free(getS(ss,i)->name);
        free(getS(ss,i));
    }
    free(ss->a);
    free(ss);
}

// Add a state, if not already defined.
state *addS(states *ss, char *name) {
    for (int i = 0; i < lengthS(ss); i++) {
        if (strcmp(name, getS(ss,i)->name) == 0) return getS(ss,i);
    }
    char *copy = malloc(strlen(name) + 1);
    strcpy(copy, name);
    state *s = malloc(sizeof(state));
    s->name = copy;
    resizeS(ss, 1);
    ss->a[lengthS(ss)-1] = s;
    return s;
}

// --------- Rules -------------------------------------------------------------

// A rule has a line number, lookahead flag, base and target states, type
// (or NULL) and a set of patterns.
typedef struct {
    int row, look; state *base, *target; char *type; tokens *patterns;
} rule;

typedef struct { int length, max; rule **a; } rules;

// Create a rule list.
rules *newR() {
    rules *rs = malloc(sizeof(rules));
    *rs = (rules) { .length = 0, .max = 8, .a = malloc(8 * sizeof(rule *)) };
    return rs;
}

int lengthR(rules *rs) { return rs->length; }
rule *getR(rules *rs, int i) { return rs->a[i]; }

void resizeR(rules *rs, int n) {
    rs->length += n;
    if (rs->length <= rs->max) return;
    while (rs->length > rs->max) rs->max = rs->max * 3 / 2;
    rs->a = realloc(rs->a, rs->max * sizeof(char *));
}

rule *addR(rules *rs) {
    rule *r = malloc(sizeof(rule));
    *r = (rule) { .patterns = newT() };
    resizeR(rs, 1);
    rs->a[lengthR(rs)-1] = r;
    return r;
}

void freeR(rules *rs) {
    for (int i = 0; i < rs->length; i++) {
        freeT(getR(rs,i)->patterns, false);
        free(getR(rs,i));
    }
    free(rs->a);
    free(rs);
}

// ---------- Scanner ----------------------------------------------------------

struct scanner { tokens *store, *patterns; states *ss;  rules *rs; };
typedef struct scanner scanner;

scanner *newScanner() {
    scanner *sc = malloc(sizeof(scanner));
    sc->store = newT();
    sc->patterns = newT();
    sc->ss = newS();
    sc->rs = newR();
    return sc;
}

void freeScanner(scanner *sc) {
    freeT(sc->store, true);
    freeT(sc->patterns, false);
    freeS(sc->ss);
    freeR(sc->rs);
    free(sc);
}

//==============================================================================


/*


*/

/*
//
void readRule(scanner *sc, strings *words, int row) {
    sc->rules = ensureRules(sc->rules, 1);
    if (words->length < 2) crash("rule on line %d too short", row);
    string base = words->a[0];
    char c = sc->store->a[base.offset];
    if (c < 'a' || c > 'z') crash("bad rule on line %d", row);
    rule *r = &sc->rules->a[sc->rules->length++];
    r->row = row;
    r->base = base;
    r->look = lookahead(words);
}*/

// Check whether a rule ends with + and remove it.
bool lookahead(tokens *words) {
    int n = lengthT(words);
    char *last = getT(words, n-1);
    if (strcmp(last, "+") != 0) return false;
    resizeT(words, -1);
    return true;
}

// Read a single rule from the tokens on a line.
void readRule(scanner *sc, tokens *ts, int row) {
    rule *r = addR(sc->rs);
    r->row = row;
    r->look = lookahead(ts);
    /*
    r->type = type(tokens);
    findOrAddString(rs->types, r->type);
    n = countStrings(tokens);
    if (n < 2) crash("rule on line %d too short", row);
    char *b = getString(tokens, 0);
    char *t = getString(tokens, n-1);
    if (! islower(b[0])) crash("bad state name %s on line %d", b, row);
    if (! islower(t[0])) crash("bad state name %s on line %d", t, row);
    r->base = b;
    r->target = t;
    r->patterns = newStrings();
    char all[] = "\\1..\\127";
    if (n == 2) readPattern(rs, row, r, all);
    else for (int i = 1; i < n - 1; i++) {
        readPattern(rs, row, r, getString(tokens,i));
    }
    */
}

// Convert a line into a list of words, or NULL for a comment line.
tokens *readLine(scanner *sc, char *line, int row) {
    int start = 0, end = 0, len = strlen(line);
    tokens *words = newT();
    while (start < len) {
        end = start;
        while (end < len && line[end] != ' ') end++;
        line[end] = '\0';
        char *t = findOrAlloc(sc->store, &line[start]);
        findOrAdd(words, t);
        start = end + 1;
        while (start < len && line[start] == ' ') start++;
    }
    return words;
}

// Process the file contents, a line at a time.
void readText(scanner *sc, char *source) {
    int p = 0, row = 1, n = strlen(source);
    for (int i = 0; i < n; i++) {
        if (source[i] != '\n') continue;
        source[i] = '\0';
        while (source[p] == ' ') p++;
        if (source[p] != '\0' && isalnum(source[p])) {
            printf("L %s\n", &source[p]);
            tokens *words = readLine(sc, &source[p], row);
            readRule(sc, words, row);
            freeT(words, false);
        }
        p = i + 1;
        row++;
        if (row > 7) break;
    }
}

#ifdef TESTscan

int main() {
    char *s = readFile("../c.txt");
    scanner *sc = newScanner();
    readText(sc, s);
//    printf("S=%s", s);
    freeScanner(sc);
    free(s);
    return 0;
}

#endif
