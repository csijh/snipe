// The Snipe editor is free and open source, see licence.txt.
// Standalone scanner to test language definitions.
#include "../src/style.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//#include <stdint.h>
//#include <stddef.h>
#include <string.h>
//#include <ctype.h>
#include <assert.h>

// Maximum sizes supported. Increase as necessary.
enum { STATES = 500, PATTERNS = 1024, TEXT = 32768, TOKENS = 400, LINES = 200 };

// An action has a target state and two token styles, one to terminate the
// current token before the pattern currently being matched, and one after. A
// style SKIP means 'this action is not relevant in the current state', and a
// style MORE means 'continue the current token without terminating it'.
enum { SKIP = POINT, MORE = SELECT };
struct action { short target; style before, after; };
typedef struct action action;

// A scanner has a table of actions indexed by state and pattern number. It has
// a patterns array of length 128 containing the pattern number of the patterns
// which start with a particular ASCII character. It has an offsets array of
// length 128 containing the corresponding offsets into the text. It has a text
// array containing the patterns, each preceded by its length in one byte. The
// patterns are sorted, longest first where one is prefix of the other, and the
// patterns starting with each character are followed by an empty pattern to act
// as a default. For tracing, there is also an array of state names.
struct scanner {
    int nStates, nPatterns;
    action *table;
    short *patterns;
    short *offsets;
    char *text;
    char **states;
};
typedef struct scanner scanner;

scanner *newScanner() {
    scanner *sc = calloc(sizeof(scanner), 1);
    sc->table = malloc(STATES * PATTERNS *sizeof(action));
    sc->patterns = malloc(128 * sizeof(short));
    sc->offsets = malloc(128 * sizeof(short));
    sc->text = malloc(TEXT);
    sc->states = malloc(STATES * sizeof(char *));
    return sc;
}

void freeScanner(scanner *sc) {
    free(sc->table);
    free(sc->patterns);
    free(sc->offsets);
    free(sc->text);
    free(sc->states);
    free(sc);
}

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
void readLines(char *text, char **lines) {
    int cn = 0, ln = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\r' && text[i] != '\n') continue;
        text[i] = '\0';
        if (text[i+1] == '\n') i++;
        lines[ln++] = &text[cn];
        cn = i + 1;
    }
    lines[ln] = NULL;
}

// Split a line into tokens at the spaces.
static void readTokens(char *line, char **tokens) {
    while (line[0] == ' ') line++;
    int n = strlen(line) + 1;
    int cn = 0, tn = 0;
    for (int i = 0; i < n; i++) {
        if (line[i] != ' ' && line[i] != '\0') continue;
        line[i] = '\0';
        if (line[cn] != '\0') tokens[tn++] = &line[cn];
        while (line[i+1] == ' ') i++;
        cn = i + 1;
    }
    tokens[tn] = NULL;
}

// Replace __ by _ and _ by space in a token, in place.
void unescape(char *token) {
    int n = strlen(token);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (token[i] == '_' && token[i+1] == '_') { i++; token[j++] = '_'; }
        else if (token[i] == '_') token[j++] = ' ';
        else token[j++] = token[i];
    }
    token[j] = '\0';
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

// Expand a range x..y into an explicit series of one-character tokens.
void expandRange(char **tokens, int t) {
    char *range = tokens[t];
    int extra = range[3] - range[0];
    int n = 0;
    while (tokens[n] != NULL) n++;
    for (int i = n; i > t; i--) tokens[i + extra] = tokens[i];
    for (int ch = range[0]; ch <= range[3]; ch++) {
        tokens[t++] = &singles[2*ch];
    }
}

char ***readRules(char *text) {
    char **lines = malloc(LINES * sizeof(char *));
    readLines(text, lines);
    char ***rules = malloc(LINES * sizeof(char **));
    int i = 0;
    while (lines[i] != NULL) {
        rules[i] = malloc(TOKENS * sizeof(char *));
        readTokens(lines[i], rules[i]);
        i++;
    }
    rules[i] = NULL;
    free(lines);
    return rules;
}

// Search for a string in a list.
int search(char **xs, char *s) {
    for (int i = 0; xs[i] != NULL; i++) {
        if (strcmp(xs[i], s) == 0) return i;
    }
    return -1;
}

void testReadLines() {
    char **lines = malloc(LINES * sizeof(char *));
    char text[] = "abc\ndef\n";
    readLines(text, lines);
    assert(strcmp(lines[0], "abc") == 0);
    assert(strcmp(lines[1], "def") == 0);
    assert(lines[2] == NULL);
    char text2[] = "abc\r\ndef\r\n";
    readLines(text2, lines);
    assert(strcmp(lines[0], "abc") == 0);
    assert(strcmp(lines[1], "def") == 0);
    assert(lines[2] == NULL);
    free(lines);
}

void testReadTokens() {
    char **tokens = malloc(TOKENS * sizeof(char *));
    char text[] = "a bb ccc dddd";
    readTokens(text, tokens);
    assert(strcmp(tokens[0], "a") == 0);
    assert(strcmp(tokens[1], "bb") == 0);
    assert(strcmp(tokens[2], "ccc") == 0);
    assert(strcmp(tokens[3], "dddd") == 0);
    assert(tokens[4] == NULL);
    char text2[] = " a   bb  ccc      dddd   ";
    readTokens(text2, tokens);
    assert(strcmp(tokens[0], "a") == 0);
    assert(strcmp(tokens[1], "bb") == 0);
    assert(strcmp(tokens[2], "ccc") == 0);
    assert(strcmp(tokens[3], "dddd") == 0);
    assert(tokens[4] == NULL);
    free(tokens);
}

void testUnescape() {
    char token[] = "__x_y__z_";
    unescape(token);
    assert(strcmp(token, "_x y_z ") == 0);
}

void testExpandRange() {
    char **tokens = malloc(TOKENS * sizeof(char *));
    char line[] = "s a..c t";
    readTokens(line, tokens);
    assert(! isRange(tokens[0]));
    assert(isRange(tokens[1]));
    expandRange(tokens, 1);
    assert(strcmp(tokens[0], "s") == 0);
    assert(strcmp(tokens[1], "a") == 0);
    assert(strcmp(tokens[2], "b") == 0);
    assert(strcmp(tokens[3], "c") == 0);
    assert(strcmp(tokens[4], "t") == 0);
    assert(tokens[5] == NULL);
    free(tokens);
}

int main() {
    makeSingles();
    testReadLines();
    testReadTokens();
    testUnescape();
    testExpandRange();
}

/*
-------------------------------------------------------------------------------
// Add a string to a list.
static void add(strings *xs, char *s) {
    int n = length(xs);
    resize(xs, n + 1);
    S(xs)[n] = s;
}

// Expand a range x..y into an explicit series of one-character tokens.
static void expandRange(strings *tokens, char *range) {
    if (singles[0] == NULL) {
        for (int i = 0; i < 128; i++) {
            singleSpace[2*i] = (char) i;
            singles[i] = &singleSpace[2*i];
        }
    }
    for (int ch = range[0]; ch <= range[3]; ch++) {
        add(tokens, singles[ch]);
    }
}

// Replace __ by _ and _ by space, in place.
static void unescape(char *token) {
    int n = strlen(token);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (token[i] == '_' && token[i+1] == '_') { i++; token[j++] = '_'; }
        else if (token[i] == '_') token[j++] = ' ';
        else token[j++] = token[i];
    }
    token[j] = '\0';
}

// Split a line into a sequence of tokens, dealing with underscores,
// expanding ranges and adding "" for missing actions.
static void readLine(char *line, strings *tokens) {
    strings *words = splitWords(line);
    int n = length(words);
    for (int i = 0; i < n; i++) {
        char *word = S(words)[i];
        unescape(word);
        if (strlen(word) == 4 && word[1] == '.' && word[2] == '.') {
            expandRange(tokens, word);
        }
        else add(tokens, word);
    }
    char *last = S(tokens)[length(tokens) - 1];
    if (isalpha(last[0])) add(tokens, empty);
    freeList(words);
}

// -----------------------------------------------------------------------------
// Find a string in a list, adding it if necessary.
int find(strings *xs, char *x) {
    int i = search(xs, x);
    if (i >= 0) return i;
    add(xs, x);
    return length(xs) - 1;
}

// Find the state names in a rule.
static void findStates(strings *tokens, strings *states) {
    find(states, S(tokens)[0]);
    find(states, S(tokens)[length(tokens) - 2]);
}

// Find the patterns in a rule.
static void findPatterns(strings *tokens, strings *patterns) {
    int n = length(tokens);
    for (int i = 1; i < n - 2; i++) find(patterns, S(tokens)[i]);
}

// -----------------------------------------------------------------------------
// Check if string s is a prefix of string t.
static inline bool prefix(char const *s, char const *t) {
    return strncmp(s, t, strlen(s)) == 0;
}

// Compare two strings in ascii order, except prefer longer strings.
static int compare(char *s, char *t) {
    int c = strcmp(s, t);
    if (c < 0 && prefix(s, t)) return 1;
    if (c > 0 && prefix(t, s)) return -1;
    return c;
}

// Sort the list of patterns, with longer patterns preferred.
static void sort(strings *patterns) {
    int n = length(patterns);
    for (int i = 1; i < n; i++) {
        char *s = S(patterns)[i];
        int j = i - 1;
        while (j >= 0 && compare(S(patterns)[j], s) > 0) {
            S(patterns)[j + 1] = S(patterns)[j];
            j--;
        }
        S(patterns)[j + 1] = s;
    }
}

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
    if (act[0] == '\0') style = MORE;
    else if (act[0] == '<') style |= BEFORE;
    else if (act[0] != '>') printf("Unknown action %s\n", act);
    action *actions = table[row];
    if (n == 3) {
        int col = search(patterns, "");
        assert(col >= 0);
        if (actions[col].style == SKIP) {
            actions[col].style = style;
            actions[col].target = target;
        }
    }
    else for (int i = 1; i < n - 2; i++) {
        char *s = S(line)[i];
        int col = search(patterns, s);
        assert(col >= 0);
        if (actions[col].style != SKIP) continue;
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
        for (int c = 0; c < width; c++) table[r][c].style = SKIP;
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
            if (act.style == SKIP) continue;
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
            if (st == MORE) printf("\n");
            else if ((st & BEFORE) != 0) printf("    <%s\n", styleName(base));
            else printf("    >%s\n", styleName(base));
        }
        if (st == MORE || s == i) continue;
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
