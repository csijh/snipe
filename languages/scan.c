// The Snipe editor is free and open source, see licence.txt.
// Provide a standalone scanner to test and compile language definitions.
#include "style.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// ---------- Arrays ----------------------------------------------------------

// Read a file as a string. Ignore errors. Normalise line endings.
char *readFile(char const *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(size + 1);
    fread(data, 1, size, file);
    data[size] = '\0';
    fclose(file);
    for (int i = 0; data[i] != '\0'; i++) {
        if (data[i] != '\r') continue;
        if (data[i+1] == '\n') data[i] = ' ';
        data[i] = '\n';
    }
    return data;
}

// Split a string in place into a NULL-terminated array of lines.
char **splitLines(char *s) {
    int n = 0;
    for (int i = 0; s[i] != '\0'; i++) if (s[i] == '\n') n++;
    char **lines = malloc((n+1) * sizeof(char *));
    n = 0;
    lines[n] = &s[0];
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != '\n') continue;
        s[i] = '\0';
        n++;
        lines[n] = &s[i+1];
    }
    lines[n] = NULL;
    return lines;
}

// Report an error on line n.
void report(int n, char *s) {
    fprintf(stderr, "Error on line %d: %s", n, s);
    exit(1);
}

// Check a line for illegal characters.
void check(int n, char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if ((s[i] & 0x80) != 0) report(n, "non-ascii character");
        if (s[i] < ' ' || s[i] > '~') report(n, "control character");
    }
}

// Get rid of leading, trailing and multiple spaces from a string.
void despace(char *s) {
    int j = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != ' ') s[j++] = s[i];
        else if (j > 0 && s[j-1] != ' ') s[j++] = s[i];
    }
    while (j > 0 && s[j-1] == ' ') j--;
    s[j] = '\0';
}

// Split a line in place into a NULL-terminated array of words.
char **splitWords(char *s) {
    int n = 0;
    for (int i = 0; s[i] != '\0'; i++) if (s[i] == ' ') n++;
    char **words = malloc((n+2) * sizeof(char *));
    n = 0;
    words[n] = &s[0];
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != ' ') continue;
        s[i] = '\0';
        n++;
        words[n] = &s[i+1];
    }
    words[n+1] = NULL;
    return words;
}

// Convert an array of lines into an array of arrays of words.
char ***makeRules(char **lines) {
    int n = 0;
    for (int i = 0; lines[i] != NULL; i++) n++;
    char ***rules = malloc((n+1) * sizeof(char **));
    for (int i = 0; lines[i] != NULL; i++) {
        rules[i] = splitWords(lines[i]);
    }
    rules[n] = NULL;
    free(lines);
    return rules;
}

enum { MaxTags = 100,  MaxStates = 1000 };

// Create an empty set of strings.
char **empty(int max) {
    char **set = malloc((max+1) * sizeof(char *));
    set[0] = NULL;
    return set;
}

// Add a string to a set of strings.
void add(char **set, char *s, int max, int ln) {
    int n;
    for (n = 0; set[n] != NULL; n++) {
        if (strcmp(set[n], s) == 0) return;
    }
    n++;
    if (n > max) report(ln, "too many strings");
    set[n] = s;
    set[n+1] = NULL;
}

int main() {
    char *text = readFile("c.txt");
    printf("Chars: %lu\n", strlen(text));
    char **lines = splitLines(text);
    int n = 0;
    while (lines[n] != NULL) n++;
    printf("Lines: %d\n", n);
    char s[20] = " a  bb ccc ";
    despace(s);
    printf("(%s)\n", s);
    char **words = splitWords(s);
    for (int i = 0; words[i] != NULL; i++) {
        printf("%d: %s\n", i, words[i]);
    }
    printf("%d %d\n", MaxTags, MaxStates);
}

/*

// extract tags: (add GAP, NEWLINE, BADCH)
// extract states: starting states then continuing states

// An action has a target state and two token styles, one to terminate the
// current token before the pattern currently being matched, and one after. A
// style Skip means 'this action is not relevant in the current state'.
enum { Skip = CountStyles };
typedef unsigned short ushort;
struct action { style before, after; ushort target; };
typedef struct action action;

// A scanner has a table of actions indexed by state and pattern number. It has
// an indexes array of length 128 containing the pattern number of the patterns
// which start with a particular ASCII character. It has an offsets array of
// length 128 containing the corresponding offsets into the symbols. It has a
// symbols array containing the pattern strings, each preceded by its
// length in one byte. The symbols are sorted, longer first where one is a
// prefix of the other, and the symbols starting with each character are
// followed by an empty string to act as a default. For tracing, there are also
// arrays of original text, style names, state names, and patterns.
struct scanner {
    int nStates, nPatterns;
    action *table;
    ushort *indexes;
    ushort *offsets;
    char *symbols;
    char *text;
    char **styles;
    char **states;
    char **patterns;
};
typedef struct scanner scanner;

// ----------------------------------------------------------------------------
// Implement lists of pointers as arrays, preceded by the length.

// The maximum length of any list. Increase as necessary.
enum { MAX = 1000 };

void *newList() {
    void **xs = malloc((MAX+1) * sizeof(void *));
    xs[0] = (void *) 0;
    return xs + 1;
}

void freeList(void *xs) {
    void **ys = (void **) xs;
    free(ys - 1);
}

int length(void *xs) {
    void **ys = (void **) xs;
    return (intptr_t) ys[-1];
}

void setLength(void *xs, int n) {
    assert(n <= MAX);
    void **ys = (void **) xs;
    ys[-1] = (void *) (intptr_t) n;
}

void add(void *xs, void *x) {
    void **ys = (void **) xs;
    ys[length(ys)] = x;
    setLength(ys, length(ys) + 1);
}

// ----------------------------------------------------------------------------
// Read in, tokenize, and normalize a language file into rules.

// Split a line into tokens at the spaces.
char **readTokens(char *line) {
    char **tokens = newList();
    while (line[0] == ' ') line++;
    int n = strlen(line) + 1;
    int cn = 0;
    for (int i = 0; i < n; i++) {
        if (line[i] != ' ' && line[i] != '\0') continue;
        line[i] = '\0';
        if (line[cn] != '\0') add(tokens, &line[cn]);
        while (i < n-1 && line[i+1] == ' ') i++;
        cn = i + 1;
    }
    return tokens;
}

// Recognize "s _ t" as matching a space and "s t" as matching the empty string.
void normalize(char **tokens) {
    int n = length(tokens);
    if (n == 3 && strcmp(tokens[1], "_") == 0) {
        tokens[1] = " ";
    }
    else if (n == 2) {
        setLength(tokens, 3);
        tokens[2] = tokens[1];
        tokens[1] =  "";
    }
}

// Check if a token is a range.
bool isRange(char *token) {
    return (strlen(token) == 4 && token[1] == '.' && token[2] == '.');
}

// Create 128 one-character strings, of two bytes each.
char singles[256];
void makeSingles() {
    for (int i = 0; i < 128; i++) {
        singles[2*i] = (char) i;
        singles[2*i+1] = '\0';
    }
}

// Expand each range x..y into an explicit series of one-character tokens.
void expandRanges(char **tokens) {
    makeSingles();
    char **work = newList();
    for (int i = 0; i < length(tokens); i++) {
        char *token = tokens[i];
        if (! isRange(token)) add(work, token);
        else {
            for (int ch = token[0]; ch <= token[3]; ch++) {
                add(work, &singles[2*ch]);
            }
        }
    }
    setLength(tokens, 0);
    for (int i = 0; i < length(work); i++) add(tokens, work[i]);
    freeList(work);
}

// Expand s:X at the start to s X and Y:t at the end to Y t, with More as the
// defaults for X and Y.
void expandStyles(char **tokens) {
    char **work = newList();
    add(work, tokens[0]);
    char *p = strchr(tokens[0], ':');
    if (p == NULL) add(work, "More");
    else {
        *p = '\0';
        add(work, p + 1);
    }
    int n = length(tokens);
    for (int i = 1; i < n - 1; i++) add(work, tokens[i]);
    p = strchr(tokens[n-1], ':');
    if (p == NULL) {
        add(work, "More");
        add(work, tokens[n-1]);
    }
    else {
        *p = '\0';
        add(work, tokens[n-1]);
        add(work, p + 1);
    }
    setLength(tokens, 0);
    for (int i = 0; i < length(work); i++) add(tokens, work[i]);
    freeList(work);
}

char ***readRules(char *text) {
    char **lines = readLines(text);
    char ***rules = newList();
    for (int i=0; i<length(lines); i++) {
        char **tokens = readTokens(lines[i]);
        if (length(tokens) == 0) continue;
        normalize(tokens);
        expandRanges(tokens);
        expandStyles(tokens);
        add(rules, tokens);
    }
    freeList(lines);
    return rules;
}

// ----------------------------------------------------------------------------
// Collect up all the distinct state names, and distinct pattern strings.

// Search for a string in a list.
int search(char **xs, char *x) {
    for (int i = 0; i < length(xs); i++) {
        if (strcmp(xs[i], x) == 0) return i;
    }
    return -1;
}

// Find a string in a list, adding it if necessary.
void find(char **xs, char *x) {
    int i = search(xs, x);
    if (i < 0) add(xs, x);
}

// Find all the state names in the rules.
static char **findStates(char ***rules) {
    char **states = newList();
    for (int i = 0; i < length(rules); i++) {
        char **tokens = rules[i];
        int n = length(tokens);
        find(states, tokens[0]);
        find(states, tokens[n-1]);
    }
    return states;
}

// Find all the patterns in the rules.
static char **findPatterns(char ***rules) {
    char **patterns = newList();
    for (int i = 0; i < length(rules); i++) {
        char **tokens = rules[i];
        int n = length(tokens);
        for (int j = 2; j < n-2; j++) find(patterns, tokens[j]);
    }
    return patterns;
}

// -----------------------------------------------------------------------------
// Sort the patterns, with longer strings coming before their prefixes. Expand
// the list of patterns so that there is an empty string after each run of
// strings with the same first character. Find the indexes into the patterns of
// the runs starting with each ASCII character. Compress the patterns into a
// single character array, with a leading lemgth byte rather than a null
// terminator for each pattern. Find the offsets in the symbols array for the
// run starting with each character.

// Check if string s is a prefix of string t.
static inline bool prefix(char const *s, char const *t) {
    return strncmp(s, t, strlen(s)) == 0;
}

// Compare two strings in ascii order, except prefer longer strings to prefixes.
int compare(char *s, char *t) {
    int c = strcmp(s, t);
    if (c < 0 && prefix(s, t)) return 1;
    if (c > 0 && prefix(t, s)) return -1;
    return c;
}

// Sort the list of patterns, with the prefix rule.
void sort(char **patterns) {
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

// Insert an empty pattern string at a given position.
void insertEmpty(char **patterns, int p) {
    int n = length(patterns);
    setLength(patterns, n+1);
    for (int i = n; i >= p; i--) patterns[i+1] = patterns[i];
    patterns[p] = "";
}

// Add an empty string when the first character of the patterns changes.
void expand(char **patterns) {
    int i = 0;
    while (i < length(patterns)) {
        char start = patterns[i][0];
        insertEmpty(patterns, i);
        i++;
        while (i < length(patterns) && patterns[i][0] == start) i++;
    }
    add(patterns, "");
}

// Find the index in the patterns of the run starting with each ASCII character.
ushort *findIndexes(char **patterns) {
    ushort *indexes = malloc(128 * sizeof(ushort));
    int here = 0;
    int nextCh = patterns[here + 1][0];
    for (int ch = 0; ch < 128; ch++) {
        if (ch < nextCh) indexes[ch] = here;
        else {
            here++;
            indexes[ch] = here;
            while (patterns[here][0] != '\0') here++;
            if (here >= length(patterns) - 1) nextCh = 128;
            else nextCh = patterns[here + 1][0];
        }
    }
    return indexes;
}

// Pack the pattern strings into a symbols array, in which each string is
// preceded by a one-byte length. Update the patterns to point into the array.
char *compress(char **patterns) {
    int n = 0;
    for (int i = 0; i < length(patterns); i++) {
        assert(strlen(patterns[i]) <= 127);
        n = n + strlen(patterns[i]) + 2;
    }
    assert(n <= 65535);
    char *symbols = malloc(n);
    n = 0;
    for (int i = 0; i < length(patterns); i++) {
        symbols[n++] = strlen(patterns[i]);
        strcpy(&symbols[n], patterns[i]);
        patterns[i] = &symbols[n];
        n = n + strlen(patterns[i]) + 1;
    }
    return symbols;
}

// Find the offset in the compressed patterns array for the n'th pattern, by
// using the lengths to skip forwards.
ushort findOffset(char *patterns, ushort n) {
    int offset = 0;
    for (int i = 0; i < n; i++) {
        offset = offset + patterns[offset] + 2;
    }
    return offset;
}

// Find the offsets in the compressed patterns array corresponding to each
// of the given 128 indexes.
ushort *findOffsets(char *patterns, ushort *indexes) {
    ushort *offsets = malloc(128 * sizeof(ushort));
    for (int ch = 0; ch < 128; ch++) {
        offsets[ch] = findOffset(patterns, indexes[ch]);
    }
    return offsets;
}

// ----------------------------------------------------------------------------
// Build a scanner from the data gathered so far. Build an array of style names,
// create a blank action table, and fill each row from a rule.

// Create the list of style names. Add "More" corresponding to CountStyles
char **makeStyles() {
    char **styles = newList();
    for (int i = 0; i < CountStyles; i++) {
        char *name = styleName(i);
        add(styles, name);
    }
    add(styles, "More");
    return styles;
}

// Create a blank action table. It is a one-dimensional array of action
// structures. It can be treated as a two-dimensional array, indexed by state
// and pattern, by casting it to type action(*)[nPatterns]. The actions are
// initialized to Skip.
action *makeTable(int nStates, int nPatterns) {
    action *rawTable = malloc(nStates * nPatterns * sizeof(action *));
    action (*table)[nPatterns] = (action(*)[nPatterns]) rawTable;
    for (int s = 0; s < nStates; s++) {
        for (int p = 0; p < nPatterns; p++) {
            table[s][p] = (action) { .before=Skip, .after=Skip, .target=0 };
        }
    }
    return rawTable;
}

// Enter a rule into the table. A rule with no patterns is a default, matching
// the empty string. The default is entered in all the empty string positions,
// so that each run of patterns starting with the same character ends with the
// default, to avoid special case default handling when the scanner executes.
void addRule(scanner *sc, char **tokens) {
    action (*table)[sc->nPatterns] = (action(*)[sc->nPatterns]) sc->table;
    int n = length(tokens);
    int state = search(sc->states, tokens[0]);
    int before = search(sc->styles, tokens[1]);
    int after = search(sc->styles, tokens[n-2]);
    int target = search(sc->states, tokens[n-1]);
    assert(state >= 0 && before >= 0 && after >= 0 && target >= 0);
    if (n == 4) {
        for (int pattern = 0; pattern < sc->nPatterns; pattern++) {
            if (sc->patterns[pattern][0] == '\0') {
                action *p = &table[state][pattern];
                if (p->before != Skip) continue;
                *p = (action) { .before=before, .after=after, .target=target };
            }
        }
    }
    else for (int i = 2; i < n - 2; i++) {
        int pattern = search(sc->patterns, tokens[i]);
        assert(pattern >= 0);
        action *p = &table[state][pattern];
        if (p->before != Skip) continue;
        *p = (action) { .before = before, .after = after, .target = target };
    }
}

scanner *newScanner(char const *path) {
    scanner *sc = malloc(sizeof(scanner));
    sc->text = readFile(path);
    char ***rules = readRules(sc->text);
    sc->states = findStates(rules);
    sc->patterns = findPatterns(rules);
    sc->styles = makeStyles();
    sort(sc->patterns);
    expand(sc->patterns);
    sc->nStates = length(sc->states);
    sc->nPatterns = length(sc->patterns);
    sc->indexes = findIndexes(sc->patterns);
    sc->symbols = compress(sc->patterns);
    sc->offsets = findOffsets(sc->symbols, sc->indexes);
    sc->table = makeTable(sc->nStates, sc->nPatterns);
    for (int i = 0; i < length(rules); i++) addRule(sc, rules[i]);
    for (int i = 0; i < length(rules); i++) freeList(rules[i]);
    freeList(rules);
    return sc;
}

void freeScanner(scanner *sc) {
    free(sc->table);
    free(sc->indexes);
    free(sc->offsets);
    free(sc->symbols);
    free(sc->text);
    freeList(sc->styles);
    freeList(sc->states);
    freeList(sc->patterns);
    free(sc);
}

// Scan a line of source text to produce a line of style bytes. At each step:
// 1) find the patterns starting with the next char.
// 2) try to match them with the text, where relevant.
// 3) move past the matched pattern.
// 4) continue or end the token and move to the next state.
void scan(scanner *sc, int row, char *line, char *styles) {
    int state = 0;
    int s = 0, i = 0, n = length(line) - 1;
    while (i < n || s < i) {
        int ch = line[i];
        if ((ch & 0x80) != 0) ch = 'A';
        int start = sc->offsets[ch];
    }
    
        int matched = empty;
        int old = i;
        for (int p = start; p < end; p++) {
            action act = sc->table[state][p];
            if (act.style == Skip) continue;
            char *pattern = sc->patterns[p];
            int len = strlen(pattern);
            if (! match(line, i, len, pattern)) continue;
            if (TRACE) printf("scan %s %s", sc->states[state], pattern);
            i = i + len;
            matched = p;
            break;
        }
        if (TRACE && matched == empty) printf("scan %s", sc->states[state]);
        action act = sc->table[state][matched];
        ushort st = act.style;
        ushort base = st & NOFLAGS;
        state = act.target;
        if (TRACE) {
            printf(" %s", sc->states[state]);
            if (st == More) printf("\n");
            else if ((st & BEFORE) != 0) printf("    <%s\n", styleName(base));
            else printf("    >%s\n", styleName(base));
        }
        if (st == More || s == i) continue;
        C(styles)[s++] = addStyleFlag(base, START);
        if ((st & BEFORE) != 0) { while (s < old) C(styles)[s++] = base; }
        else while (s < i) C(styles)[s++] = base;
    }
    C(styles)[s] = addStyleFlag(GAP, START);
    if (TRACE) printf("end state %s\n", sc->states[state]);
    sc->endStates = setEndState(sc->endStates, row, state);
    
}

// ----------------------------------------------------------------------------
// Tests.

// Test that readTokens divides a line at the spaces into tokens.
void testReadTokens() {
    char text[] = "";
    char **tokens = readTokens(text);
    assert(length(tokens) == 0);
    freeList(tokens);
    char text1[] = "a bb ccc dddd";
    tokens = readTokens(text1);
    assert(length(tokens) == 4);
    assert(strcmp(tokens[0], "a") == 0);
    assert(strcmp(tokens[1], "bb") == 0);
    assert(strcmp(tokens[2], "ccc") == 0);
    assert(strcmp(tokens[3], "dddd") == 0);
    freeList(tokens);
    char text2[] = " a   bb  ccc      dddd   ";
    tokens = readTokens(text2);
    assert(length(tokens) == 4);
    assert(strcmp(tokens[0], "a") == 0);
    assert(strcmp(tokens[1], "bb") == 0);
    assert(strcmp(tokens[2], "ccc") == 0);
    assert(strcmp(tokens[3], "dddd") == 0);
    freeList(tokens);
}

// Check that a list of strings matches a space separated description. An empty
// string in the list is represented as a question mark in the description. This
// uses readTokens to split up the description.
bool check(char **list, char *s) {
    char s2[strlen(s)+1];
    strcpy(s2, s);
    char **strings = readTokens(s2);
    bool ok = true;
    if (length(list) != length(strings)) ok = false;
    else for (int i = 0; i < length(list); i++) {
        if (list[i][0] == '\0' && strings[i][0] == '?') { }
        else if (strcmp(list[i], strings[i]) != 0) ok = false;
    }
    freeList(strings);
    return ok;
}

// Test that readLines divides lines, coping with different line endings.
void testReadLines() {
    char text[] = "abc\ndef\n";
    char **lines = readLines(text);
    assert(check(lines, "abc def"));
    freeList(lines);
    char text2[] = "abc\r\ndef\r\n";
    lines = readLines(text2);
    assert(check(lines, "abc def"));
    freeList(lines);
}

// Test that the two special cases are handled.
void testNormalize() {
    char line[] = "s _ t";
    char **tokens = readTokens(line);
    normalize(tokens);
    assert(strcmp(tokens[0], "s") == 0);
    assert(strcmp(tokens[1], " ") == 0);
    assert(strcmp(tokens[2], "t") == 0);
    freeList(tokens);
    char line2[] = "s t";
    tokens = readTokens(line2);
    normalize(tokens);
    assert(strcmp(tokens[0], "s") == 0);
    assert(strcmp(tokens[1], "") == 0);
    assert(strcmp(tokens[2], "t") == 0);
    freeList(tokens);
}

// Test that ranges are expanded to one-character strings.
void testExpandRanges() {
    char line[] = "s a..c t";
    char **tokens = readTokens(line);
    expandRanges(tokens);
    assert(check(tokens, "s a b c t"));
    freeList(tokens);
}

// Test that the before and after styles for a rule are extracted, with More as
// the default.
void testExpandStyles() {
    char line[] = "s p t";
    char **tokens = readTokens(line);
    expandStyles(tokens);
    assert(check(tokens, "s More p More t"));
    freeList(tokens);
    char line2[] = "s:X p t";
    tokens = readTokens(line2);
    expandStyles(tokens);
    assert(check(tokens, "s X p More t"));
    freeList(tokens);
    char line3[] = "s p X:t";
    tokens = readTokens(line3);
    expandStyles(tokens);
    assert(check(tokens, "s More p X t"));
    freeList(tokens);
}

// Test sorting, with the prefix rule.
void testSort() {
    char line[] = "s a b aa c ba ccc sx";
    char **patterns = readTokens(line);
    sort(patterns);
    assert(check(patterns, "aa a ba b ccc c sx s"));
    freeList(patterns);
}

// Test that runs of patterns starting with the same character are divided by
// empty strings, and that findIndexes identifies the runs.
void testIndexes() {
    char line[] = "b bc d de";
    char **patterns = readTokens(line);
    expand(patterns);
    assert(check(patterns, "? b bc ? d de ?"));
    ushort *indexes = findIndexes(patterns);
    assert(indexes[0] == 0);
    assert(indexes['a'] == 0);
    assert(indexes['b'] == 1);
    assert(indexes['c'] == 3);
    assert(indexes['d'] == 4);
    assert(indexes['e'] == 6);
    assert(indexes[127] == 6);
    freeList(patterns);
    free(indexes);
}

// Test that patterns are compressed into a symbols array, with leading length
// bytes, and an updated patterns array.
void testCompress() {
    char line[] = "b bc d de";
    char **patterns = readTokens(line);
    expand(patterns);
    assert(check(patterns, "? b bc ? d de ?"));
    char *symbols = compress(patterns);
    assert(memcmp(symbols, "\0\0\1b\0\2bc\0\0\0\1d\0\2de\0\0\0", 20) == 0);
    assert(patterns[0] == &symbols[1]);
    assert(patterns[1] == &symbols[3]);
    assert(patterns[2] == &symbols[6]);
    assert(patterns[3] == &symbols[10]);
    assert(patterns[4] == &symbols[12]);
    assert(patterns[5] == &symbols[15]);
    assert(patterns[6] == &symbols[19]);
    freeList(patterns);
    free(symbols);
}

// Test that findOffsets converts indexes into offsets in the symbols array.
void testOffsets() {
    char line[] = "b bc d de";
    char **patterns = readTokens(line);
    expand(patterns);
    char *symbols = compress(patterns);
    assert(memcmp(symbols, "\0\0\1b\0\2bc\0\0\0\1d\0\2de\0\0\0", 20) == 0);
    ushort *indexes = findIndexes(patterns);
    ushort *offsets = findOffsets(symbols, indexes);
    assert(offsets[0] == 0);
    assert(offsets['a'] == 0);
    assert(offsets['b'] == 2);
    assert(offsets['c'] == 9);
    assert(offsets['d'] == 11);
    assert(offsets['e'] == 18);
    assert(offsets[127] == 18);
    freeList(patterns);
    free(indexes);
    free(symbols);
    free(offsets);
}

int main() {
    setbuf(stdout, NULL);
    testReadTokens();
    testReadLines();
    testNormalize();
    testExpandRanges();
    testExpandStyles();
    testSort();
    testIndexes();
    testCompress();
    testOffsets();
    scanner *sc = newScanner("c.txt");
    freeScanner(sc);
    printf("Tests pass\n");
}
*/
/*
// -----------------------------------------------------------------------------
// Enter a rule into the table.
static void addRule(strings *line, action **table, strings *states,
    strings *patterns, strings *styles)
{
    int n = length(line);
    ushort row = search(states, S(line)[0]);
    ushort target = search(states, S(line)[n-2]);
    char *act = S(line)[n-1];
    assert(row >= 0 && target >= 0);
    ushort style = 0;
    if (strlen(act) > 0) {
        char *styleName = &act[1];
        style = search(styles, styleName);
        if (style < 0) printf("Unknown style %s\n", styleName);
    }
    if (act[0] == '\0') style = More;
    else if (act[0] == '<') style |= BEFORE;
    else if (act[0] != '>') printf("Unknown action %s\n", act);
    action *actions = table[row];
    if (n == 3) {
        int col = search(patterns, "");
        assert(col >= 0);
        if (actions[col].style == Skip) {
            actions[col].style = style;
            actions[col].target = target;
        }
    }
    else for (int i = 1; i < n - 2; i++) {
        char *s = S(line)[i];
        int col = search(patterns, s);
        assert(col >= 0);
        if (actions[col].style != Skip) continue;
        actions[col].style = style;
        actions[col].target = target;
    }
}

// Create a blank table.
static action **makeTable(strings *states, strings *patterns)
{
    int height = length(states);
    int width = length(patterns);
    action **table = malloc(height * sizeof(action *));
    for (int r = 0; r < height; r++) {
        table[r] = malloc(width * sizeof(action));
        for (int c = 0; c < width; c++) table[r][c].style = Skip;
    }
    return table;
}

// Fill a table from the rules.
static void fillTable(action **table, strings **rules, strings *states,
    strings *patterns, strings *styles)
{
    for (int i = 0; rules[i] != NULL; i++) {
        strings *rule = rules[i];
        addRule(rule, table, states, patterns, styles);
    }
}

void changeLanguage(scanner *sc, char *lang) {
    char *content = readLanguage(lang);
    strings *states = newStrings();
    strings *patterns = newStrings();
    find(patterns, "");
    strings *lines = splitLines(content);
    strings **rules = splitTokens(lines);
    findNames(rules, states, patterns);
    freeData(sc);
    sc->chars = content;
    sort(patterns);
    strings *styles = makeStyles();
    action **table = makeTable(states, patterns);
    fillTable(table, rules, states, patterns, styles);
    sc->table = table;
    sc->height = length(states);
    sc->width = length(patterns);
    sc->offsets = makeOffsets(patterns);
    sc->states = S(states);
    sc->states = realloc(sc->states, sc->height * sizeof(char *));
    free(states);
    sc->patterns = S(patterns);
    sc->patterns = realloc(sc->patterns, sc->width * sizeof(char *));
    free(patterns);
    for (int i = 0; rules[i] != NULL; i++) freeList(rules[i]);
    free(rules);
    freeList(lines);
    freeList(styles);
}

static bool TRACE;

// Match part of a list against a string.
static inline bool match(chars *line, int i, int n, char *pattern) {
    return strncmp(C(line) + i, pattern, n) == 0;
}

// Scan a line of source text to produce a line of style bytes. At each step:
// 1) find the range, start <= x < end, of patterns starting with the next char.
// 2) try to match the ones that are relevant.
// 3) move past the matched pattern.
// 4) if no relevant patterns match, use the empty pattern as a default.
// 5) continue or end the token and move to the next state.
void scan(scanner *sc, int row, chars *line, chars *styles) {
    resize(styles, length(line));
    int state = (row == 0) ? 0 : sc->endStates[row - 1];
    int s = 0, i = 0, n = length(line) - 1;
    int empty = sc->offsets[127];
    while (i < n || s < i) {
        int ch = C(line)[i];
        if ((ch & 0x80) != 0) ch = 'A';
        int start = sc->offsets[ch];
        int end = sc->offsets[ch + 1];
        int matched = empty;
        int old = i;
        for (int p = start; p < end; p++) {
            action act = sc->table[state][p];
            if (act.style == Skip) continue;
            char *pattern = sc->patterns[p];
            int len = strlen(pattern);
            if (! match(line, i, len, pattern)) continue;
            if (TRACE) printf("scan %s %s", sc->states[state], pattern);
            i = i + len;
            matched = p;
            break;
        }
        if (TRACE && matched == empty) printf("scan %s", sc->states[state]);
        action act = sc->table[state][matched];
        ushort st = act.style;
        ushort base = st & NOFLAGS;
        state = act.target;
        if (TRACE) {
            printf(" %s", sc->states[state]);
            if (st == More) printf("\n");
            else if ((st & BEFORE) != 0) printf("    <%s\n", styleName(base));
            else printf("    >%s\n", styleName(base));
        }
        if (st == More || s == i) continue;
        C(styles)[s++] = addStyleFlag(base, START);
        if ((st & BEFORE) != 0) { while (s < old) C(styles)[s++] = base; }
        else while (s < i) C(styles)[s++] = base;
    }
    C(styles)[s] = addStyleFlag(GAP, START);
    if (TRACE) printf("end state %s\n", sc->states[state]);
    sc->endStates = setEndState(sc->endStates, row, state);
}

// Check a list of tokens produced by readLine.
static bool checkReadLine(char *t, int n, char *e[n]) {
    char tc[100];
    strcpy(tc, t);
    strings *ts = newStrings();
    readLine(tc, ts);
    if (length(ts) != n) return false;
    for (int i = 0; i < n; i++) {
        char *t = S(ts)[i];
        if (strcmp(t, e[i]) != 0) return false;
    }
    freeList(ts);
    return true;
}

// Check that readLine generates a suitable list of tokens (state, before-style,
// patterns, after-style, state).
static void testReadLine() {
    assert(checkReadLine("s s", 3, (char *[]) {"s","s",""}));
    assert(checkReadLine("s _ s", 4, (char *[]) {"s"," ","s",""}));
    assert(checkReadLine("s A..B s", 5, (char *[]) {"s","A","B","s",""}));
    assert(checkReadLine("s _..! s", 5, (char *[]) {"s"," ","!","s",""}));
    assert(checkReadLine("s ! s >OP", 4, (char *[]) {"s","!","s",">OP"}));
    assert(checkReadLine("s ! s <OP", 4, (char *[]) {"s","!","s","<OP"}));
    assert(checkReadLine("s s >OP", 3, (char *[]) {"s","s",">OP"}));
}

static bool check(scanner *sc, int row, char *l, char *expect) {
    chars *line = newChars();
    resize(line, strlen(l));
    strncpy(C(line), l, strlen(l));
    chars *styles = newChars();
    scan(sc, row, line, styles);
    for (int i = 0; i < length(line); i++) {
        char st = C(styles)[i];
        char letter = styleName(clearStyleFlags(st))[0];
        if (! hasStyleFlag(st, START)) letter = tolower(letter);
        C(styles)[i] = letter;
    }
    bool matched = match(styles, 0, length(line), expect);
    freeList(line);
    freeList(styles);
    return matched;
}

static void test() {
    scanner *sc = newScanner();
    assert(check(sc, 0, "x\n", "WG"));
    assert(check(sc, 0, "x y\n", "WGWG"));
    assert(check(sc, 0, "xy, z\n", "WwSGWG"));
    changeLanguage(sc, "c");
    assert(check(sc, 0, "x\n", "IG"));
    assert(check(sc, 0, "x y\n", "IGIG"));
    assert(check(sc, 0, "int x;\n", "KkkGISG"));
    assert(check(sc, 0, "intx;\n", "IiiiSG"));
    assert(check(sc, 0, "//xy\n", "NnnnG"));
    assert(check(sc, 0, "/" "* one\n", "CcGCccG"));
    assert(check(sc, 1, "x*" "/y\n", "CccIG"));
    freeScanner(sc);
}

// Trace scanning of a problem example.
static void trace() {
    scanner *sc = newScanner();
    changeLanguage(sc, "c");
    assert(check(sc, 0, "//x\n", "NnnG"));
    assert(check(sc, 0, "//x \n", "NnnGG"));
    freeScanner(sc);
}

int main(int n, char *args[n]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    testReadLine();
    TRACE = false;
    if (TRACE) trace();
    else test();
    printf("Scan module OK\n");
    return 0;
}
*/
