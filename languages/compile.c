// Snipe language compiler. Free and open source. See licence.txt.

// Compile a language description in .../rules.txt into a scanner table in
// .../table.txt. The program interpret.c can be used to test the table.

// TODO THIN THIS. ADD description syntax stuff. Scanning produces a tag for
// each byte of text. For the first byte of a token, the tag is the type of
// token. For subsequent bytes, the tag indicates a continuation character of
// the token, or a continuation byte of a character (or start of joining
// character forming a grapheme). In the full Snipe scanner, the tag for a token
// contains flags to indicate that the token type is being overridden by ? or *
// or =, and the tag for a space contains the scanner state, to allow for
// incremental re-scanning from that point.

// TODO:
// 1) check progress: initial state must be safe
// 2) every start state with a lookahead rule must use a safe state as a target.
// 3) add x? for every x as lookahead patterns TICK
// 4) if s has lookaheads, have s _ s' where s' is lookahead only copy.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// MAX is the general limit on array sizes. It can be increased as necessary.
// Arrays are not resized, but they are checked for overflow.
enum { MAX = 1000 };

// State indexes and tags are stored in bytes.
typedef unsigned char byte;

// A tag is a symbol or upper case character representing a token type, or SKIP
// to mean ignore a table entry as not relevant to the current state, or MORE to
// mean continue the current token.
enum { SKIP = '~', MORE = '-'};

// --------- Structures --------------------------------------------------------

// Pattern, state, and rule structures all have a name as the first field, so
// are all compatible with this structure.
struct item { char *name; };
typedef struct item item;

// A pattern has a name and an index.
struct pattern {
    char *name;
    int index;
};
typedef struct pattern pattern;

// A state has a name and an index. The start flag indicates whether it is a
// start state or a continuation state. A line number records the line where the
// flag was set, or 0 if the flag is undecided.
struct state {
    char *name;
    int index;
    bool start;
    int line;
};
typedef struct state state;

// A rule has a name and a line number. It also has a base state and a target
// state, a list of patterns, and a tag. If the original rule had no token type,
// the tag in the structure is set to MORE.
struct rule {
    char *name;
    int line;
    state *base, target;
    pattern **patterns;
    byte tag;
};
typedef struct rule rule;

// A language description has lists of rules, states and patterns.
struct language {
    int nrules, nstates, npatterns;
    rule **rules;
    state **states;
    char **patterns;
};
typedef struct language language;

// Each table entry contains a tag as an action, and a target state. A token
// type tag means terminate the current token with that type. Tag SKIP means
// ignore the entry as not relevant to the current state, and MORE means
// continue the current token.
struct entry {
    byte action, target;
};
typedef struct entry entry;

// TODO three of these.

state *findState(language *lang, char *name) {
    for (int i = 0; i < lang->nstates; i++) {
        if (strcmp(name, lang->states[i]->name) == 0) return i;
    }
    state *s = malloc(sizeof(state));
    *s = (state) {
        .name = name, .index = lang->nstates, .start = false, .line = 0
    };
    lang->states[lang->nstates++] = s;
    return s;
}

// Create a new pattern, state or rule structure of the given size.
void *newItem(char *name, int size) {
    item *p = malloc(size);
    p->name = name;
    return p;
}

// ----- Lists -----------------------------------------------------------------

// Crash with an error message and possibly a line number or string.
void crash(char *e, int line, char const *s) {
    fprintf(stderr, "Error:");
    fprintf(stderr, " %s", e);
    if (line > 0) fprintf(stderr, " on line %d", line);
    if (strlen(s) > 0) fprintf(stderr, " (%s)", s);
    fprintf(stderr, "\n");
    exit(1);
}

// TODO: findState. Doesn't have to create list, because done in creating lang.
// If doesn't exist, creates and initializes.

// Allocate a new list of pointers, with the length at index -1.
void *newList() {
    void **list = malloc((MAX+1) * sizeof(void *));
    list[0] = (void *) 0;
    return list + 1;
}

// Find the length of a list of pointers.
int length(void *list) {
    return (intptr_t) ((void **) list)[-1];
}

// Add a pointer to a list, returning its index.
int add(void *list, void *p) {
    void **plist = (void **) list;
    int n = length(plist);
    if (n >= MAX) crash("List overflow", 0, "");
    plist[n] = p;
    plist[-1] = (void *) (intptr_t) (n+1);
    return n;
}

// Find a named item in a list and return its index or -1.
int find(void *list, char *name) {
    item **items = (item **) list;
    for (int i = 0; i < length(items); i++) {
        if (strcmp(name, items[i]->name) == 0) return i;
    }
    return -1;
}

// ----- Lines and tokens -----------------------------------------------------

// Validate a line. Check it is ASCII only. Convert '\t' or '\r' to a space. Ban
// other control characters.
void validateLine(int n, char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        unsigned char ch = line[i];
        if (ch == '\t' || ch == '\r') line[i] = ' ';
        else if (ch >= 128) crash("non-ASCII character", n, "");
        else if (ch < 32 || ch > 126) crash("control character", n, "");
    }
}

// Split the text into a list of lines, replacing each newline by a null. Add
// a blank line at the start, so that the index is the same as the line number.
char **splitLines(char *text) {
    int p = 0, n = 1;
    char **lines = (char **) newList();
    add(lines, "");
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i]= '\0';
        validateLine(n, &text[p]);
        add(lines, &text[p]);
        n++;
        p = i + 1;
    }
    return lines;
}

// Space for a one-character string for each ASCII character.
char singles[128][2];

// Space for a three-character lookahead string for each ASCII character.
char triples[128][4];

// Create the singles and triples strings.
void makeSinglesTriples() {
    for (int i = 0; i < 128; i++) {
        singles[i][0] = (char)i;
        singles[i][1] = '\0';
        triples[i][0] = '.';
        triples[i][1] = '.';
        triples[i][2] = (char)i;
        triples[i][3] = '\0';
    }
}

// Check whether a pattern string is a range of characters or a lookahead range.
bool isRange(char *s) {
    int n = strlen(s);
    if (n != 4 && n != 6) return false;
    if (n == 6 && (s[0] != '.' || s[1] != '.')) return false;
    if (n == 6) s = s + 2;
    return s[1] == '.' && s[2] == '.';
}

// Expand a range x..y or ..x..y into multiple explicit tokens.
// Add to the tokens list.
void expandRange(char *range, char *tokens[]) {
    char start, end;
    bool lookahead = strlen(range) == 6;
    if (lookahead) { start = range[2]; end = range[5]; }
    else { start = range[0]; end = range[3]; }
    for (int ch = start; ch <= end; ch++) {
        if (lookahead) add(tokens, triples[ch]);
        else add(tokens, singles[ch]);
    }
}

// ---------- Process rules ----------------------------------------------------

// Find or create a state.
state *findState(state *states, char *name) {
    int i = find(states, name);
    if (i >= 0) return states[i];
    state *s = malloc(sizeof(state));
    i = add(states, s);
    *s = (state) { .name = name, .index = i, .start = false, .line = 0 };
    return s;
}

// Find or create a pattern.
state *findPattern(pattern *patterns, char *name) {
    int i = find(patterns, name);
    if (i >= 0) return patterns[i];
    pattern *p = malloc(sizeof(pattern));
    i = add(patterns, p);
    *p = (pattern) { .name = name, .index = i };
    return p;
}

// Find a tag character from a rule's token type.
byte findTag(int line, char *name) {
    if ('A' <= name[0] && name[0] <= 'Z') return name[0];
    if (strlen(name) == 1) crash("bad token type", line, name);
    char ok[] = "()[]{}<>#/\\^$*'\"@=:?";
    if (strchr(ok, name[0]) == NULL) crash("bad token type", line, name);
    return name[0];
}

// Read a rule from a list of tokens from a given line, and check its format.
rule *readRule(language *lang, int line, char *tokens[]) {
    if (length(tokens) == 0) return;
    char ch = tokens[0][0];
    if (ch < 'a' || ch > 'z') return;
    rule *r = malloc(sizeof(rule));
    r->line = line;
    r->base = findState(lang->states, tokens[0]);
    int n = length(tokens);
    char *last = tokens[n-1];
    if ('a' <= last[0] && last[0] <= 'z') {
        if (n < 2) crash("rule too short", line, "");
        r->target = findState(lang, last);
        r->tag = MORE;
    }
    else {
        if (n < 3) crash("rule too short", line, "");
        char *t = tokens[n-2];
        if (t[0] < 'a' || t[0] > 'z') crash("expecting target state", line, t);
        r->target = findState(lang->states, t);
        r->tag = findTag(line, last);
        n--;
    }
    char **patterns = newList();
    for (int i = 1; i < n-1; i++) {
        pattern *p = findPattern(lang->patterns, tokens[i]);
        add(patterns, tokens[i]);
    }
}

// ----- File handling ---------------------------------------------------------

// Read a text file as a string, adding a final newline if necessary, and a null
// terminator. Use binary mode, so that the file size equals the bytes read in.
char *readFile(char const *path, char *text) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("can't read file", 0, path);
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("can't find file size", 0, path);
    char *text = malloc(size+2);
    int n = fread(text, 1, size, file);
    if (n != size) crash("can't read file", 0, path);
    if (n > 0 && text[n - 1] != '\n') text[n++] = '\n';
    text[n] = '\0';
    fclose(file);
    return text;
}

// ----- Lists -----------------------------------------------------------------

//==============================================================================
/*


// Validate the tokens for a line. If there is no tag, add a MORE ('-') tag.
void validateTokens(int i, char *ts[]) {
    int n = length(ts);
    if (n < 3) crash("rule too short", i, "");
    char *last = ts[n-1];
    if ('a' <= last[0] && last[0] <= 'z') {
        add(ts, singles[MORE]);
        return;
    }
    char *target = ts[n-2];
    if ('a' <= target[0] && target[0] <= 'z') return;
    crash("expecting target state", i, target);
}

// Split each line into an array of tokens, replacing spaces by null characters
// as necessary. Expand ranges into explicit one- or two-character tokens. If
// there is no token type, add an explicit MORE token.
void splitTokens(char *lines[], char *tokens[][SMALL]) {
    int nlines = length(lines);
    for (int i = 0; i < nlines; i++) {
        char *line = lines[i];
        int start = 0, end = 0, len = strlen(line);
        while (line[start] == ' ') start++;
        while (start < len) {
            end = start;
            while (line[end] != ' ' && line[end] != '\0') end++;
            line[end] = '\0';
            char *pattern = &line[start];
            if (isRange(pattern)) expandRange(pattern, tokens[i]);
            else add(tokens[i], pattern);
            start = end + 1;
            while (start < len && line[start] == ' ') start++;
        }
        validateTokens(i, tokens[i]);
    }
}

// Gather start states, that can occur between tokens, so they come first. The
// position of the state name in the array is the state number. Make sure there
// are no more than 32 of them, so the state number can be packed into 5 bits.
int gatherStartStates(int n, char *tokens[n][SMALL], char *states[]) {
    find(states, tokens[0][0]);
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        char *tag = tokens[i][t-1];
        if (strlen(tag) == 0) continue;
        char *target = tokens[i][t-2];
        find(states, target);
    }
    int n = length(states);
    if (n > 32) crash("more than 32 start states", 0, "");
    return n;
}

// Gather distinct state names from the tokens. Make sure the total number
// states is at most 62, so they can be numbered using 0..9a..zA..Z. Return the
// number of start states.
int gatherStates(int n, char *tokens[n][SMALL], char *states[]) {
    int nstarts = gatherStartStates(n, tokens, states);
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        find(states, tokens[i][0]);
        find(states, tokens[i][t-2]);
    }
    if (length(states) > 62) crash("more than 62 states", 0, "");
    return nstarts;
}

// Gather pattern strings, including single character lookahead strings.
void gatherPatterns(int n, char *tokens[n][SMALL], char *patterns[]) {
    find(patterns, " ?");
    find(patterns, "\n?");
    for (int ch = '!'; ch <= '~'; ch++) find(patterns, triples[ch]);
    for (int i = 0; i < n; i++) {
        for (int t = 1; t < length(tokens[i])-2; t++) {
            find(patterns, tokens[i][t]);
        }
    }
}

// ----- Sorting --------------------------------------------------------------

// Compare two strings in ASCII order, except prefer longer strings and
// non-lookaheads.
int compare(char *s, char *t) {
    for (int i = 0; ; i++) {
        if (s[i] == '\0' && i > 0 && t[i] == '?') return -1;
        if (s[i] == '\0' && t[i] != '\0') return 1;
        if (s[i] == '\0') return 0;
        if (t[i] == '\0' && i > 0 && s[i] == '?') return 1;
        if (t[i] == '\0') return -1;
        if (i > 0 && s[i] == '?') return 1;
        if (i > 0 && t[i] == '?') return -1;
        if (s[i] < t[i]) return -1;
        if (s[i] > t[i]) return 1;
    }
}

// Sort the patterns.
void sort(char *patterns[]) {
    int n = length(patterns);
    for (int i = 1; i < n; i++) {
        char *s = patterns[i];
        int j = i - 1;
        while (j >= 0 && compare(patterns[j], s) > 0) {
            patterns[j + 1] = patterns[j];
            j--;
        }
        patterns[j + 1] = s;
    }
}
*/
// ----- Checking -------------------------------------------------------------
/* TODO

A pattern is x..y or ..xx or ..x..y with any other appearance of .. illegal

Any rule with a token type has a start state as its target. A rule with a
lookahead pattern, or a default rule, with a token type has a continuation
state as its base.

A rule with a normal pattern but no token type has a continuation state as
its target.

A rule with a lookahead pattern, or a default rule, and no token type has
base and target states which are both start states or both continuation
states. The target state must make progress by matching at least part of
each lookahead pattern handled explicitly or implicitly by the rule.
*/
/*
void check(scanner *sc) {

}


// ----- Building -------------------------------------------------------------

// Fill a non-default rule into the table.
void fillRule(
    entry table[][MAX], char *tokens[], char *states[], char *patterns[]
) {
    int n = length(tokens);
    byte action = (byte) tokens[n-1][0];
    byte state = (byte) find(states, tokens[0]);
    byte target = (byte) find(states, tokens[n-2]);
    for (int i = 1; i < n-2; i++) {
        short p = (short) find(patterns, tokens[i]);
        byte oldAction = table[state][p].action;
        if (oldAction != SKIP) continue;
        table[state][p].action = action;
        table[state][p].target = target;
    }
}

// Fill a default rule into the table.
void fillDefault(
    entry table[][MAX], char *tokens[], char *states[], char *patterns[]
) {
    byte action = (byte) tokens[2][0];
    byte state = (byte) find(states, tokens[0]);
    byte target = (byte) find(states, tokens[1]);
    char s[3] = "x?";
    for (int ch = 0; ch < 128; ch++) {
        if (ch < ' ' && ch != '\n') continue;
        if (ch == 127) continue;
        s[0] = ch;
        int p = find(patterns, s);
        table[state][p].action = action;
        table[state][p].target = target;
    }
}
*/
/*
// Check for any missing defaults, and complain.
void checkMissing(
    entry table[][MAX], char *states[], char *patterns[]
) {
    for (int s = 0; s < length(states); s++) {
        int n = length(patterns) - 1;
        if (table[s][n].action != SKIP) continue;
        crash("Default rule needed for state", 0, states[s]);
    }
}
*/
/*
// Enter rules into the table.
void fillTable(
    entry table[][MAX], int n, char *tokens[n][SMALL],
    char *states[], char *patterns[]
) {
    for (int s=0; s<SMALL; s++) for (int p=0; p<MAX; p++) {
        table[s][p].action = SKIP;
    }
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        if (t == 3) fillDefault(table, tokens[i], states, patterns);
        else fillRule(table, tokens[i], states, patterns);
    }
//    checkMissing(table, states, patterns);
}

scanner *buildScanner(char const *path) {
    scanner *sc = calloc(1, sizeof(scanner));
    readFile(path, sc->text);
    splitLines(sc->text, sc->lines);
    makeSinglesTriples();
    splitTokens(sc->lines, sc->tokens);
    int nlines = length(sc->lines);
    sc->nstarts = gatherStates(nlines, sc->tokens, sc->states);
    gatherPatterns(nlines, sc->tokens, sc->patterns);
    sort(sc->patterns);
    sc->nstates = length(sc->states);
    sc->npatterns = length(sc->patterns);
    fillTable(sc->table, nlines, sc->tokens, sc->states, sc->patterns);
    return sc;
}

// Write out a row of state names (for tracing) then a row per pattern with the
// pattern string on the end, and finally the defaults row.
void writeScanner(scanner *sc, char const *path) {
    FILE *fp = fopen(path, "w");
    char *stateLabels =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < sc->nstates; i++) {
        char *s = sc->states[i];
        if (i > 0) fprintf(fp, " ");
        fprintf(fp, "%s", s);
    }
    fprintf(fp, "\n");
    for (int p = 0; p < sc->npatterns; p++) {
        for (int s = 0; s < sc->nstates; s++) {
            char action = (char) sc->table[s][p].action;
            char target = stateLabels[sc->table[s][p].target];
            fprintf(fp, "%c%c ", action, target);
        }
        if (strlen(sc->patterns[p]) == 0) fprintf(fp, " default\n");
        else fprintf(fp, " %s\n", sc->patterns[p]);
    }
    fclose(fp);
}

// ----- Testing --------------------------------------------------------------

// Check two NULL terminated arrays of strings are equal.
bool eq(char *as[], char *bs[]) {
    if (length(as) != length(bs)) return false;
    for (int i = 0; i < length(as); i++) {
        if (strcmp(as[i],bs[i]) != 0) return false;
    }
    return bs[length(as)] == NULL;
}

void testSplitLines() {
    char s[] = "abc\ndef\n\nghi\n";
    char *lines[10] = {NULL};
    char *expect[] = {"abc", "def", "ghi", NULL};
    splitLines(s, lines);
    assert(eq(lines, expect));
}

void testSplitTokens() {
    char s1[20] = "s a b c t", s2[20] = " s  \\s \\b  t  ", s3[20] = "s a..c t";
    char s4[20] = "s a t X";
    char *lines[5] = {s1, s2, s3, s4, NULL};
    char *expect[4][8] = {
        {"s", "a", "b", "c", "t", "-", NULL},
        {"s", "\\s", "\\b", "t", "-", NULL},
        {"s", "a", "b", "c", "t", "-", NULL},
        {"s", "a", "t", "X", NULL},
    };
    char *tokens[4][SMALL] = {{NULL},{NULL},{NULL},{NULL}};
    makeSinglesTriples();
    splitTokens(lines, tokens);
    assert(eq(tokens[0], expect[0]));
    assert(eq(tokens[1], expect[1]));
    assert(eq(tokens[2], expect[2]));
    assert(eq(tokens[3], expect[3]));
}

void testGatherStates() {
    char *ts[2][SMALL] = {{"s0","?","s1","X",NULL}, {"s0","s2","X",NULL}};
    char *states[10] = {NULL};
    char *expect[] = {"s0", "s1", "s2", NULL};
    gatherStates(2, ts, states);
    assert(eq(states, expect));
}

void testGatherPatterns() {
    char *ts[2][SMALL] = {{"s","x","s","X",NULL}, {"s","y","s","X",NULL}};
    char *patterns[200] = {NULL};
    gatherPatterns(2, ts, patterns);
    assert(strcmp(patterns[96],"x") == 0);
    assert(strcmp(patterns[97],"y") == 0);
}

void testSort() {
    assert(compare("!","<") == -1);
    assert(compare("<=","<") == -1);
    assert(compare("<","<=") == 1);
    assert(compare("<","<?") == -1);
    assert(compare("<?","<") == 1);
    assert(compare("<=","<?") == -1);
    assert(compare("<?","<=") == 1);

    char *patterns[] = { "<?", "", "<", "<=", "!", NULL, NULL };
    char *expect[] = {"!", "<=", "<", "<?", "", NULL };
    sort(patterns);
    printf("p=%s %s\n", patterns[2], patterns[3]);
    assert(eq(patterns, expect));
}
*/
int main(int n, char const *args[n]) {
    /*
    testSplitLines();
    testSplitTokens();
    testGatherStates();
    testGatherPatterns();
    testSort();
    */
    /*
    if (n != 2) crash("Use: ./compile language", 0, "");
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    scanner *sc = buildScanner(path);
    sprintf(path, "%s/table.txt", args[1]);
    writeScanner(sc, path);
    free(sc);
    */
    return 0;
}
