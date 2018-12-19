// The Snipe editor is free and open source, see licence.txt.
// Provide a standalone scanner to test and compile language definitions.
#include "../src/style.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// An action has a target state and two token styles, one to terminate the
// current token before the pattern currently being matched, and one after. A
// style Skip means 'this action is not relevant in the current state', and a
// style More means 'continue the current token without terminating it'.
enum { Skip = POINT, More = SELECT };
struct action { short target; style before, after; };
typedef struct action action;

// A scanner has a table of actions indexed by state and pattern number. It has
// an indexes array of length 128 containing the pattern number of the patterns
// which start with a particular ASCII character. It has an offsets array of
// length 128 containing the corresponding offsets into the symbols. It has a
// symbols array containing the text of the patterns, each preceded by its
// length in one byte. The patterns are sorted, longest first where one is a
// prefix of the other, and the patterns starting with each character are
// followed by an empty pattern to act as a default. For tracing, there is also
// an array of state names.
struct scanner {
    int nStates, nPatterns;
    action *table;
    short *indexes;
    short *offsets;
    char *symbols;
    char **states;
};
typedef struct scanner scanner;

scanner *newScanner() {
    scanner *sc = calloc(sizeof(scanner), 1);
    /*
    sc->table = malloc(STATES * PATTERNS *sizeof(action));
    sc->indexes = malloc(128 * sizeof(short));
    sc->offsets = malloc(128 * sizeof(short));
    sc->symbols = malloc(TEXT);
    sc->states = malloc(STATES * sizeof(char *));
    */
    return sc;
}

void freeScanner(scanner *sc) {
    free(sc->table);
    free(sc->indexes);
    free(sc->offsets);
    free(sc->symbols);
    free(sc->states);
    free(sc);
}

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

// Read a file as a string. Ignore errors.
char *readFile(char const *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(size + 1);
    fread(data, 1, size, file);
    data[size] = '\0';
    fclose(file);
    return data;
}

// Split the text into lines.
char **readLines(char *text) {
    char **lines = newList(sizeof(char *));
    int cn = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\r' && text[i] != '\n') continue;
        text[i] = '\0';
        if (text[i+1] == '\n') i++;
        add(lines, &text[cn]);
        cn = i + 1;
    }
    return lines;
}

// Split a line into tokens at the spaces.
char **readTokens(char *line) {
    char **tokens = newList(sizeof(char *));
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

// Replace __ by _ and _ by space in a list of tokens, in place.
void unescape(char **tokens) {
    for (int i = 0; i < length(tokens); i++) {
        char *token = tokens[i];
        int n = strlen(token);
        int j = 0;
        for (int i = 0; i < n; i++) {
            if (token[i] == '_' && token[i+1] == '_') { i++; token[j++] = '_'; }
            else if (token[i] == '_') token[j++] = ' ';
            else token[j++] = token[i];
        }
        token[j] = '\0';
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
        unescape(tokens);
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
    char **states = newList(sizeof(char *));
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
// Sort the patterns, with prefixes coming after their parents. Expand the list
// of patterns so that there is an empty string after each run of strings with
// the same first character. Find the indexes into the patterns of the runs
// starting with each ASCII character.

// Check if string s is a prefix of string t.
static inline bool prefix(char const *s, char const *t) {
    return strncmp(s, t, strlen(s)) == 0;
}

// Compare two strings in ascii order, except prefer longer strings to prefixes.
static int compare(char *s, char *t) {
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
short *findIndexes(char **patterns) {
    short *indexes = malloc(128 * sizeof(short));
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

// Compress the patterns into a symbols array, in which each string is stored
// as a one-byte length, then the characters, and no null terminator.

// ----------------------------------------------------------------------------
// Build a scanner from the data gathered so far.
// Build an action table from the rules and patterns.

/*
// Convert the patterns into a symbols array.
char *symbolize(char **patterns) {
    char *symbols = newList(sizeof(char));
    int n = length(patterns);
    for (int i = 0; i < n; i++) {
        char *pattern = patterns[i];
        int len = strlen(pattern);
        assert(len < 128);
        int k = length(symbols);
        addLength(symbols, len + 1);
        symbols[len++] = len;
        for (int i = 0; i < len; i++) symbols[len++] = pattern[i];
    }
}
*/
scanner *build(char const *path) {
    char *text = readFile(path);
    char ***rules = readRules(text);
    char **states = findStates(rules);
    char **patterns = findPatterns(rules);
    sort(patterns);
    expand(patterns);
    scanner *sc = newScanner();
    sc->nStates = length(states);
    sc->nPatterns = length(patterns);
    sc->indexes = findIndexes(patterns);
    return sc;
}

// ----------------------------------------------------------------------------
// Test.

// Check that a list of strings matches a space separated string. An empty
// string in the list is represented as a question mark in the string.
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
    assert(check(tokens, "a bb ccc dddd"));
    freeList(tokens);
}

void testUnescape() {
    char line[] = "__x_y__z_";
    char **tokens = readTokens(line);
    unescape(tokens);
    assert(strcmp(tokens[0], "_x y_z ") == 0);
    freeList(tokens);
}

void testExpandRanges() {
    char line[] = "s a..c t";
    char **tokens = readTokens(line);
    expandRanges(tokens);
    assert(check(tokens, "s a b c t"));
    freeList(tokens);
}

void testExpandStyles() {
    char line[] = "s t";
    char **tokens = readTokens(line);
    expandStyles(tokens);
    assert(check(tokens, "s More More t"));
    freeList(tokens);
    char line2[] = "s:X t";
    tokens = readTokens(line2);
    expandStyles(tokens);
    assert(check(tokens, "s X More t"));
    freeList(tokens);
    char line3[] = "s X:t";
    tokens = readTokens(line3);
    expandStyles(tokens);
    assert(check(tokens, "s More X t"));
    freeList(tokens);
}

void testSort() {
    char line[] = "s a b aa c ba ccc sx";
    char **patterns = readTokens(line);
    sort(patterns);
    assert(check(patterns, "aa a ba b ccc c sx s"));
    freeList(patterns);
}

void testIndexes() {
    char line[] = "b bc d de";
    char **patterns = readTokens(line);
    expand(patterns);
    assert(check(patterns, "? b bc ? d de ?"));
    short *indexes = findIndexes(patterns);
    assert(indexes[0] == 0);
    assert(indexes['a'] == 0);
    assert(indexes['b'] == 1);
    assert(indexes['c'] == 3);
    assert(indexes['d'] == 4);
    assert(indexes['e'] == 6);
    assert(indexes[127] == 6);
}

int main() {
    setbuf(stdout, NULL);
    testReadLines();
    testReadTokens();
    testUnescape();
    testExpandRanges();
    testExpandStyles();
    testSort();
    testIndexes();
    build("c.txt");
    printf("Tests pass\n");
}
/*
// -----------------------------------------------------------------------------
// Create the list of style names.
static strings *makeStyles() {
    strings *styles = newStrings();
    for (int i = 0; i < COUNT_STYLES; i++) {
        char *name = styleName(i);
        add(styles, name);
    }
    return styles;
}

// Create an offsets array. For each ascii character c, offsets[c] to
// offsets[c+1] gives the range of patterns starting with that character.
static short *makeOffsets(strings *patterns) {
    short *offsets = malloc(128 * sizeof(short));
    int ch = 0, n = length(patterns);
    for (int i = 0; i < n; i++) {
        char *pattern = S(patterns)[i];
        int first = pattern[0];
        while (ch < first) offsets[ch++] = i - 1;
        offsets[ch++] = i;
        while (i < n - 1 && S(patterns)[i + 1][0] == first) i++;
    }
    while (ch < 128) offsets[ch++] = n - 1;
    return offsets;
}

// Enter a rule into the table.
static void addRule(strings *line, action **table, strings *states,
    strings *patterns, strings *styles)
{
    int n = length(line);
    short row = search(states, S(line)[0]);
    short target = search(states, S(line)[n-2]);
    char *act = S(line)[n-1];
    assert(row >= 0 && target >= 0);
    short style = 0;
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

// Split each line into a list of tokens.
static strings **splitTokens(strings *lines) {
    strings **rules = malloc((length(lines) + 1) * sizeof(strings *));
    int j = 0;
    for (int i = 0; i < length(lines); i++) {
        char *line = S(lines)[i];
        if (line[0] == '-') break;
        if (! isalpha(line[0])) continue;
        rules[j] = newStrings();
        readLine(line, rules[j]);
        j++;
    }
    rules[j] = NULL;
    return rules;
}

// Extract all the state names and patterns from the lines.
static void findNames(strings *rules[], strings *states,
    strings *patterns)
{
    for (int i = 0; rules[i] != NULL; i++) {
        findStates(rules[i], states);
        findPatterns(rules[i], patterns);
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
        short st = act.style;
        short base = st & NOFLAGS;
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
