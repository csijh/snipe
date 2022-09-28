#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// Report error in the style of printf, and exit.
void crash(char const *fmt, ...) {
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

// Report warning in the style of printf.
void warn(char const *fmt, ...) {
    fprintf(stderr, "Warning: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// ---------- Lists -----------------------------------------------------------

// A list is an array of object pointers, preceded by its length and capacity.
typedef struct { int length, size; } list;

void *newList() {
    list *ls = malloc(sizeof(list) + 8 * sizeof(void *));
    *ls = (list) { .length = 0, .size = 8 };
    return ls + 1;
}

void freeList(void *p) { free((list *)p - 1); }

int length(void *p) { list *ls = ((list *)p) - 1; return ls->length; }

// Change length by n, assuming there is room, returning the old length,
int lengthen(void *p, int n) {
    list *ls = ((list *)p) - 1;
    ls->length += n;
    return ls->length - n;
}

void clear(void *p) { lengthen(p, -length(p)); }

// Add an item, assuming there is room.
void push(void *p, void *x) {
    void **items = (void **) p;
    items[lengthen(p,1)] = x;
}

void pop(void *p) { lengthen(p, -1); }

// Ensure room for n more objects, returning possibly reallocated list. Call on
// an original list, before passing it to functions which might lengthen it.
void *resize(void *p, int n) {
    list *ls = ((list *)p) - 1;
    if (ls->length + n <= ls->size) return p;
    while (ls->length + n > ls->size) ls->size = ls->size * 3 / 2;
    ls = realloc(ls, sizeof(list) + ls->size * sizeof(void *));
    return ls + 1;
}

// ---------- Read -------------------------------------------------------------

// Read source file and normalise. Check for controls or Unicode.
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

// Split a line into words, in place.
char **splitWords(char *line) {
    char **words = newList();
    int start = 0, end = 0, len = strlen(line);
    while (line[start] == ' ') start++;
    while (start < len) {
        end = start;
        while (end < len && line[end] != ' ') end++;
        words = resize(words, 1);
        push(words, &line[start]);
        start = end + 1;
        while (start < len && line[start] == ' ') start++;
    }
    return words;
}

// Split the source into lines, in place. Each line is a list of words.
char ***splitLines(char *source) {
    char ***lines = newList();
    int start = 0, len = strlen(source);
    for (int i = 0; i < len; i++) {
        if (source[i] != '\n') continue;
        source[i]= '\0';
        lines = resize(lines, 1);
        push(lines, splitWords(&source[start]));
        start = i + 1;
    }
    return lines;
}

// ---------- Tokenize ---------------------------------------------------------

// Tokens are unique strings, which can be compared by pointer.
typedef char *token;

// Turn a string into a token by lookup in the list of all tokens. Assume
// the list has room for one more token.
token find(token *all, char *s) {
    for (int i = 0; i < length(all); i++) {
        if (strcmp(s, all[i]) == 0) return all[i];
    }
    push(all, s);
    return s;
}

// Add a token to a set of tokens. Assume the set has room for one more token.
void add(token *ts, token t) {
    for (int i = 0; i < length(ts); i++) {
        if (t == ts[i]) return;
    }
    push(ts, t);
}

// Tokens are stored in-place in the file contents which are read in, with the
// exception of implicit one-character patterns.
static char singles[128][2];

// Convert words to tokens.
token **tokenize(char ***lines) {
    token *all = newList();
    for (int i = 0; i < 128; i++) {
        singles[i][0] = i;
        all = resize(all, 1);
        push(all, singles[i]);
    }
    for (int i = 0; i < length(lines); i++) {
        char **line = lines[i];
        for (int j = 0; j < length(line); j++) {
            char *s = line[j];
            all = resize(all, 1);
            token t = find(all, s);
            line[j] = t;
        }
    }
    freeList(all);
    return lines;
}

// --------- Normalize ---------------------------------------------------------

// Check and normalize patterns, assuming the line contains only a base state
// and pattern strings, not a target state, token type or lookahead flag.
// Normalize character code patterns and check the syntax of ranges.
void normalizePatterns(token *line, int row) {
    for (int i = 1; i < length(line); i++) {
        token pattern = line[i];
        char *dots = strstr(pattern,"..");
        if (dots != NULL) {
            if (strlen(pattern) == 2) continue;
            if (strlen(pattern) == 4 && dots == pattern+1) continue;
            crash("bad range on line %d %s", row, pattern);
        }
        bool allDigits = true;
        for (int j = 0; j < strlen(pattern); j++) {
            if (! isdigit(pattern[j])) allDigits = false;
        }
        if (! allDigits) continue;
        if (strcmp(pattern,"0") == 0) crash("nul character code, line %d", row);
        if (pattern[0] == '0') crash("bad character code, line %d", row);
        int code = atoi(pattern);
        if (code >= 128) crash("character code too high, line %d", row);
        if (code != 10 && code != 32) {
            warn("character code not 10 or 32, line %d", row);
        }
        line[i] = singles[code];
    }
}

// Check and normalize a line of tokens. Clear the line if it is a comment, add
// a minus sign if there is no plus sign, and add an X (extend) if there is no
// token type.
token *normalizeLine(token *line, int row) {
    printf("LEN %d\n", length(line));
    printf("0: %s", line[0]);
    if (length(line) == 0) return line;
    char ch = line[0][0];
    if (! isalnum(ch)) { clear(line); return line; }
    if (ch < 'a' || ch > 'z') crash("bad base state on line %d", row);
    token flag = singles['-'];
    if (length(line) < 3) crash("rule on line %d too short", row);
    token last = line[length(line) - 1];
    if (last == singles['+']) {
        flag = singles['+'];
        pop(line);
    }
    last = line[length(line) - 1];
    token type = singles['X'];
    if ('A' <= last[0] && last[0] <= 'Z') {
        type = last;
        pop(line);
    }
    if (length(line) < 3) crash("rule on line %d too short", row);
    normalizePatterns(line, row);
    last = line[length(line) - 1];
    ch = last[0];
    if (ch < 'a' || ch > 'z') crash("bad target state on line %d", row);
    line = resize(line, 2);
    push(line, type);
    push(line, flag);
    return line;
}

void normalize(token **lines) {
    for (int i = 0; i < length(lines); i++) {
        lines[i] = normalizeLine(lines[i], i+1);
    }
}

// After normalizing, use these functions.
token base(token *rule) { return rule[0]; }
token target(token *rule) { return rule[length(rule)-3]; }
token type(token *rule) { return rule[length(rule)-2]; }
bool lookahead(token *rule) { return rule[length(rule)-1][0] == '+'; }
bool isDefault(token *rule) { return strcmp(rule[1], "..") == 0; }

// --------- States ------------------------------------------------------------

// A state ...
typedef struct { token name; bool hasDefault, isStarting, isContinuing; } state;

// Find a state with a given name.
state *findState(state **states, token name) {
    for (int i = 0; i < length(states); i++) {
        state *s = states[i];
        if (s->name == name) return s;
    }
    return NULL;
}

// Add a state with given name.
void addState(state **states, token name) {
    if (findState(states, name) != NULL) return;
    state *s = malloc(sizeof(state));
    *s = (state) { .name = name };
    push(states,s);
}

// Gather list of states from rules.
state **gatherStates(token **rules) {
    state **states = newList();
    states = resize(states, length(rules));
    for (int i = 0; i < length(rules); i++) {
        if (length(rules[i]) == 0) continue;
        addState(states, base(rules[i]));
    }
    return states;
}

// Run through rules, checking targets and setting state flags.
void checkStates(state **states, token **rules) {
    for (int i = 0; i < length(rules); i++) {
        token *rule = rules[i];
        if (length(rule) == 0) continue;
        token t = target(rule);
        if (findState(states,t) == NULL) crash("no target state, line %d", i+1);
        state *b = findState(states, base(rule));
        if (b->hasDefault) warn("unreachable rule, line %d", i+1);
        if (isDefault(rule)) b->hasDefault = true;

    }
}

// ---------- Correctness ------------------------------------------------------

// Check if a state exists.
bool isState(token s, state **states) {
    for (int i = 0; i < length(states); i++) {
        if (s == states[i]->name) return true;
    }
    return false;
}

// Check that targets exist.
void checkTargets(token **rules, state **states) {
    for (int i = 0; i < length(rules); i++) {
        token *rule = rules[i];
        token t = target(rule);
        if (! isState(t, states)) crash("unknown state %s (line %d)", t, i+1);
    }
}

// Check a state to see if it has a default rule.
void checkDefault(state *st, token **rules) {
    for (int i = 0; i < length(rules); i++) {
        token *rule = rules[i];
        if (base(rule) != st->name) continue;
        if (isDefault(rule)) {
            if (st->hasDefault) warn("unreachable rule, line %d", i+1);
            st->hasDefault = true;
        }
    }
}
/*
// Check whether rule r shows that state s is a starting state, i.e. s is the
// base of the first rule, or the target of a rule with a token type.
static bool showsStarting(state *s, rule *r) {
    if (strcmp(s->name, r->base) == 0 && r->row == 1) return true;
    if (strcmp(s->name, r->target) == 0 && isTerminating(r)) return true;
    return false;
}

// Check whether rule r shows that state s is a continuing state, i.e. s is the
// base of a terminating lookahead rule, or the target of a non-lookahead
// non-terminating rule.
static bool showsContinuing(state *s, rule *r) {
    if (strcmp(s->name, r->base) == 0) {
        if (r->lookahead && isTerminating(r)) return true;
    }
    if (strcmp(s->name, r->target) == 0) {
        if (! r->lookahead && ! isTerminating(r)) return true;
    }
    return false;
}
*/
// --------- Rules -------------------------------------------------------------

// A rule has a line number, lookahead flag, base and target states, type
// (or NULL) and a set of patterns.
typedef struct { int row, look; token base, *patterns, target, type; } rule;
/*
// Check whether a rule ends with + and remove it.
bool lookahead(token *words) {
    int n = length(words);
    token last = words[n-1];
    if (strcmp(last, "+") != 0) return false;
    lengthen(words, -1);
    return true;
}

// Extract the token type, if any.
token type(token *words) {
    int n = length(words);
    token last = words[n-1];
    if (! isupper(last[0])) return NULL;
    lengthen(words, -1);
    return last;
}

// Create a rule from a list of tokens from a line.
rule *readRule(int row, token *words) {
    if (length(words) < 2) crash("rule on line %d too short", row);
    rule *r = malloc(sizeof(rule));
    r->row = row;
    r->look = lookahead(words);
    r->type = type(words);
    int n = length(words);
    if (n < 2) crash("rule on line %d too short", row);
    token b = words[0];
    if (! islower(b[0])) crash("bad state name %s on line %d", b, row);
    r->base = b;
    token t = words[n-1];
    if (! islower(t[0])) crash("bad state name %s on line %d", t, row);
    r->target = t;
    r->patterns = newList();
    r->patterns = resize(r->patterns, n-2);
    lengthen(r->patterns, n-2);
    for (int i = 0; i < n-2; i++) r->patterns[i] = words[i+1];
    return r;
}
*/
// ---------- Scanner ----------------------------------------------------------

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
/*

// Read a single rule from the tokens on a line.
void readRule(scanner *sc, tokens *ts, int row) {
    rule *r = addR(sc->rs);
    findOrAddString(rs->types, r->type);
    n = countStrings(tokens);
    if (n < 2) crash("rule on line %d too short", row);
    r->base = b;
    r->target = t;
    r->patterns = newStrings();
    char all[] = "\\1..\\127";
    if (n == 2) readPattern(rs, row, r, all);
    else for (int i = 1; i < n - 1; i++) {
        readPattern(rs, row, r, getString(tokens,i));
    }
}
*/
// ---------- Scanner ----------------------------------------------------------
/*
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
*/
//==============================================================================



// Convert the source into a list of rules. ALL? SCANNER.
rule *readRules() { return NULL; }

/*
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
*/
#ifdef TESTscan2

int main() {
    char *s = readFile("../c.txt");
//    scanner *sc = newScanner();
    char ***lines = splitLines(s);
    token **lines2 = tokenize(lines);
    normalize(lines2);
    printf("#lines = %d\n", length(lines2));
    char **words = lines2[0];
    printf("#words = %d\n", length(words));
    for (int i = 0; i < length(lines2); i++) freeList(lines2[i]);
    freeList(lines2);
//    printf("S=%s", s);
//    freeScanner(sc);
    free(s);
    return 0;
}

#endif
