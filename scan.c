// The Snipe editor is free and open source, see licence.txt.

#include "scan.h"
#include "style.h"
#include "theme.h"
#include "string.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// A scanner action has a token style and a target state.
struct action { short style, target; };
typedef struct action action;

// A scanner has an array of state names, a sorted array of pattern strings, and
// a table of actions indexed by state and pattern number. The offsets array
// gives the position of the patterns that start with each character. The
// endStates array holds the final state for each line of text, to carry over to
// the next line. The chars array holds the file content, as storage space for
// the strings. It is assumed that the scanner is only called on a line when all
// previous lines have been scanned.
struct scanner {
    int height, width;
    char **states, **patterns;
    action **table;
    short *offsets;
    short *endStates;
    char *chars;
};
typedef struct scanner scanner;

// The style field in an action is set to SKIP to mean 'this pattern isn't
// relevant to this state', set to MORE to mean 'continue with the current
// token', or flagged with BEFORE to mean 'terminate the token before the string
// currently being matched, rather than after it'. Use NOFLAGS to remove flags.
enum { SKIP = 0x80, MORE = 0x40, BEFORE = 0x20, NOFLAGS = 0x1F };

// Manage a variable length array of short ints to hold scan states at the ends
// of lines, with non-negative unused entries and -1 in the last position to
// keep track of the capacity.
static short *newEndStates() {
    short *es = calloc(16, sizeof(short));
    es[15] = -1;
    return es;
}

// Set the i'th entry to e.
static short *setEndState(short *es, int i, short e) {
    if (es[i] == -1) {
        int n = 2 * i;
        es = realloc(es, n * sizeof(short));
        for (int j = i; j < n; j++) es[j] = 0;
        es[n - 1] = -1;
    }
    es[i] = e;
    return es;
}

scanner *newScanner() {
    scanner *sc = calloc(sizeof(scanner), 1);
    sc->endStates = newEndStates();
    changeLanguage(sc, "txt");
    return sc;
}

// Free the data in the scanner for a specific language.
static void freeData(scanner *sc) {
    free(sc->states);
    free(sc->patterns);
    for (int r = 0; r < sc->height; r++) free(sc->table[r]);
    free(sc->table);
    free(sc->offsets);
    free(sc->chars);
}

void freeScanner(scanner *sc) {
    freeData(sc);
    free(sc->endStates);
    free(sc);
}

// -----------------------------------------------------------------------------
// Static space for 128 one-character strings, and an empty string.
static char singleSpace[256];
static char *singles[128];
static char *empty = "";

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
        char *word = get(words, i);
        unescape(word);
        if (strlen(word) == 4 && word[1] == '.' && word[2] == '.') {
            expandRange(tokens, word);
        }
        else add(tokens, word);
    }
    char *last = get(tokens, length(tokens) - 1);
    if (isalpha(last[0])) add(tokens, empty);
    freeStrings(words);
}

// Read in a language file as a string.
static char *readLanguage(char *lang) {
    char *path = resourcePath("languages/", lang, ".txt");
    if (sizeFile(path) < 0) {
        free(path);
        path = resourcePath("languages/", "txt", ".txt");
    }
    char *content = readPath(path);
    free(path);
    normalize(content);
    return content;
}

// -----------------------------------------------------------------------------
// Search for a string in a list.
int search(strings *xs, char *s) {
    for (int i = 0; i < length(xs); i++) {
        char *si = get(xs, i);
        if (strcmp(s, si) == 0) return i;
    }
    return -1;
}

// Find a string in a list, adding it if necessary.
int find(strings *xs, char *x) {
    int i = search(xs, x);
    if (i >= 0) return i;
    add(xs, x);
    return length(xs) - 1;
}

// Find the state names in a rule.
static void findStates(strings *tokens, strings *states) {
    find(states, get(tokens, 0));
    find(states, get(tokens, length(tokens) - 2));
}

// Find the patterns in a rule.
static void findPatterns(strings *tokens, strings *patterns) {
    int n = length(tokens);
    for (int i = 1; i < n - 2; i++) find(patterns, get(tokens, i));
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
        char *s = get(patterns, i);
        int j = i - 1;
        while (j >= 0 && compare(get(patterns, j), s) > 0) {
            set(patterns, j + 1, get(patterns, j));
            j--;
        }
        set(patterns, j + 1, s);
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
        char *pattern = get(patterns, i);
        int first = pattern[0];
        while (ch < first) offsets[ch++] = i - 1;
        offsets[ch++] = i;
        while (i < n - 1 && get(patterns, i + 1)[0] == first) i++;
    }
    while (ch < 128) offsets[ch++] = n - 1;
    return offsets;
}

// Enter a rule into the table.
static void addRule(strings *line, action **table, strings *states,
    strings *patterns, strings *styles)
{
    int n = length(line);
    short row = search(states, get(line, 0));
    short target = search(states, get(line, n-2));
    char *act = get(line, n-1);
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
        char *s = get(line, i);
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
        char *line = get(lines, i);
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
    sc->states = freeze(states);
    sc->patterns = freeze(patterns);
    for (int i = 0; rules[i] != NULL; i++) freeStrings(rules[i]);
    free(rules);
    freeStrings(lines);
    freeStrings(styles);
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
        int ch = get(line,i);
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
//            printf("state %s match '%s'", sc->states[state], pattern);
            i = i + len;
            matched = p;
            break;
        }
//        if (match == empty) printf("state %s match ''", sc->states[state]);
        action act = sc->table[state][matched];
        short st = act.style;
        short base = st & NOFLAGS;
//        printf(" st=%d\n", st);
        state = act.target;
        if (st == MORE || s == i) continue;
        set(styles, s++, addStyleFlag(base, START));
        if ((st & BEFORE) != 0) { while (s < old) set(styles, s++, base); }
        else while (s < i) set(styles, s++, base);
    }
    set(styles, s, addStyleFlag(GAP, START));
    sc->endStates = setEndState(sc->endStates, row, state);
}

#ifdef test_scan

// Check a list of tokens produced by readLine.
static bool checkReadLine(char *t, int n, char *e[n]) {
    char tc[100];
    strcpy(tc, t);
    strings *ts = newStrings();
    readLine(tc, ts);
    if (length(ts) != n) return false;
    for (int i = 0; i < n; i++) {
        char *t = get(ts, i);
        if (strcmp(t, e[i]) != 0) return false;
    }
    freeStrings(ts);
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
    insert(line, 0, strlen(l), l);
    chars *styles = newChars();
    scan(sc, row, line, styles);
    for (int i = 0; i < length(line); i++) {
        char st = get(styles, i);
        char letter = styleName(clearStyleFlags(st))[0];
        if (! hasStyleFlag(st, START)) letter = tolower(letter);
        set(styles, i, letter);
    }
    bool matched = match(styles, 0, length(line), expect);
    freeChars(line);
    freeChars(styles);
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

int main(int n, char *args[n]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    testReadLine();
    test();
    printf("Scan module OK\n");
    return 0;
}

#endif
