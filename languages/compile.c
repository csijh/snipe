// Snipe language compiler. Free and open source. See licence.txt.

// Compile a language description in .../rules.txt into a scanner table in
// .../table.txt. The program interpret.c can be used to test the table.
// Scanning produces a tag for each byte of text. For the first byte of a token,
// the tag is the type of token. For subsequent bytes, the tag indicates a
// continuation character of the token, or a continuation byte of a character
// (or start of joining character forming a grapheme). In the full Snipe
// scanner, the tag for a token contains flags to indicate that the token type
// is being overridden by ? or * or =, and the tag for a space contains the
// scanner state, to allow for incremental re-scanning from that point.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// BIG is the limit on array sizes. It can be increased as necessary.
// SMALL is the limit on arrays indexed by unsigned bytes.
enum { BIG = 10000, SMALL = 256 };

// Each table entry contains a tag as an action, and a target state. A token
// type tag means terminate the current token with that type. Tag SKIP means
// ignore the entry as not relevant to the current state, and MORE means
// continue the current token.
typedef unsigned char byte;
struct entry { byte action, target; };
typedef struct entry entry;
enum { SKIP = '~', MORE = '-'};

// A scanner structure consists of the number of states and patterns (which
// determines the size of the state transition table), then the table itself,
// then a string store containing the state names followed by the patterns. The
// remaining fields are for temporary use during construction. Many are NULL
// terminated arrays of strings.
struct scanner {
    short nstates, npatterns;
    entry table[SMALL][BIG];
    char strings[BIG];

    char text[BIG];
    char *lines[BIG];
    char *tokens[BIG][SMALL];
    char *states[SMALL];
    char *patterns[BIG];
    char *temp[BIG];
    short nstrings;
};
typedef struct scanner scanner;

// ----- File handling --------------------------------------------------------

// Crash with an error message and possibly a line number or string.
void crash(char *e, int n, char const *s) {
    fprintf(stderr, "Error");
    if (n > 0) fprintf(stderr, " on line %d", n);
    fprintf(stderr, " %s\n", e);
    if (strlen(s) > 0) fprintf(stderr, "%s", s);
    fprintf(stderr, "\n");
    exit(1);
}

// Read a text file as a string, adding a final newline if necessary, and a null
// terminator. Use binary mode, so that the file size equals the bytes read in.
void readFile(char const *path, char *text) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("can't read file", 0, path);
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("can't find file size", 0, path);
    if (size >= BIG) crash("file too big", 0, path);
    int n = fread(text, 1, size, file);
    if (n != size) crash("can't read file", 0, path);
    if (n > 0 && text[n - 1] != '\n') text[n++] = '\n';
    text[n] = '\0';
    fclose(file);
}

// ----- Lists and sets of strings --------------------------------------------

// Find length of NULL terminated list or set of strings.
int length(char *strings[]) {
    int n = 0;
    while (strings[n] != NULL) n++;
    return n;
}

// Add a string to a list.
void add(char *s, char *strings[]) {
    int n = length(strings);
    strings[n++] = s;
    strings[n] = NULL;
}

// Look up a string in a set, adding it if necessary, and return its index.
int find(char *s, char *strings[]) {
    int i;
    for (i = 0; strings[i] != NULL; i++) {
        if (strcmp(s, strings[i]) == 0) return i;
    }
    strings[i] = s;
    strings[i+1] = NULL;
    return i;
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

// Split the text into an array of lines, replacing each newline by a null. Skip
// blank lines or lines starting with anything other than a lowercase letter.
void splitLines(char *text, char *lines[]) {
    int p = 0, n = 1;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i]= '\0';
        validateLine(n, &text[p]);
        if (text[p] != '\0' && 'a' <= text[p] && text[p] <= 'z') {
            add(&text[p], lines);
        }
        n++;
        p = i + 1;
    }
}

// Space for a one-character string for each ASCII character.
char singles[128][2];

// Create the single-character strings.
void makeSingles() {
    for (int i = 0; i < 128; i++) {
        singles[i][0] = (char)i;
        singles[i][1] = '\0';
    }
}

// Check whether a pattern string is a range of characters.
bool isRange(char *s) {
    return strlen(s) == 4 && s[1] == '.' && s[2] == '.';
}

// Expand a range x..y into an explicit series of one-character tokens. Add to
// the tokens array.
void expandRange(char *range, char *tokens[]) {
    for (int ch = range[0]; ch <= range[3]; ch++) {
        add(singles[ch], tokens);
    }
}

// Check if a string is a single allowable symbol (excluding . ~ - _).
bool isSymbol(char *s) {
    char ok[] = "()[]{}#<>^$*'\"@=:?";
    if (strlen(s) != 1) return false;
    if (strchr(ok, s[0]) == NULL) return false;
    return true;
}

// Validate the tokens for a line. If there is no tag, add a MORE ('-') tag.
void validateTokens(int i, char *ts[]) {
    int n = length(ts);
    if (n < 3) crash("rule too short", i, "");
    char *last = ts[n-1];
    if ('a' <= last[0] && last[0] <= 'z') {
        add("MORE", ts);
        return;
    }
    bool ok = 'A' <= last[0] && last[0] <= 'Z';
    if (! ok) ok = isSymbol(last);
    if (! ok) crash("expecting tag", i, last);
    char *target = ts[n-2];
    if ('a' <= target[0] && target[0] <= 'z') return;
    crash("expecting target state", i, target);
}

// Split each line into an array of tokens, replacing spaces by null characters
// as necessary. Expand ranges into explicit one-character tokens. If there is
// no token type, add an explicit MORE token.
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
            else add(pattern, tokens[i]);
            start = end + 1;
            while (start < len && line[start] == ' ') start++;
        }
        validateTokens(i, tokens[i]);
    }
}

// Gather states that can occur between tokens, so they come first. The position
// of the state name in the array is the state number. Make sure there are no
// more than 32 of them, so the state number can be packed into 5 bits.
void gatherStartStates(int n, char *tokens[n][SMALL], char *states[]) {
    find(tokens[0][0], states);
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        char *tag = tokens[i][t-1];
        if (strlen(tag) == 0) continue;
        char *target = tokens[i][t-2];
        find(target, states);
    }
    int s = length(states);
    if (s > 32) crash("more than 32 start states", 0, "");
}

// Gather distinct state names from the tokens. Make sure the total number
// states is at most 62, so they can be numbered using 0..9a..zA..Z.
void gatherStates(int n, char *tokens[n][SMALL], char *states[]) {
    gatherStartStates(n, tokens, states);
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        find(tokens[i][0], states);
        find(tokens[i][t-2], states);
    }
    if (length(states) > 62) crash("more than 62 states", 0, "");
}

// Gather pattern strings.
void gatherPatterns(int n, char *tokens[n][SMALL], char *patterns[]) {
    for (int i = 0; i < n; i++) {
        for (int t = 1; t < length(tokens[i])-2; t++) {
            find(tokens[i][t], patterns);
        }
    }
}

// ----- Sorting --------------------------------------------------------------

// Check if string s is a prefix of string t.
bool prefix(char const *s, char const *t) {
    return strncmp(s, t, strlen(s)) == 0;
}

// Compare two strings in ASCII order, except prefer longer strings.
int compare(char *s, char *t) {
    int c = strcmp(s, t);
    if (c < 0 && prefix(s, t)) return 1;
    if (c > 0 && prefix(t, s)) return -1;
    return c;
}

// Sort the patterns, with longer patterns preferred.
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

// Add an empty pattern string after each run of patterns starting with the same
// character.
void expandPatterns(char *patterns[], char *temp[]) {
    int n = length(patterns);
    for (int i = 0; i <= n; i++) temp[i] = patterns[i];
    int p = 0, t = 0;
    patterns[p++] = "";
    for (int i = 0; i < 128; i++) {
        if (t < n && i >= temp[t][0]) {
            while (t < n && temp[t][0] == i) patterns[p++] = temp[t++];
            patterns[p++] = "";
        }
    }
}

// ----- Building -------------------------------------------------------------

// Fill a non-default rule into the table.
void fillRule(
    entry table[][BIG], char *tokens[], char *states[], char *patterns[]
) {
    int n = length(tokens);
    byte action = (byte) tokens[n-1][0];
    byte state = (byte) find(tokens[0], states);
    byte target = (byte) find(tokens[n-2], states);
    for (int i = 1; i < n-1; i++) {
        short p = (short) find(tokens[i], patterns);
        byte oldAction = table[state][p].action;
        if (oldAction != SKIP) continue;
        table[state][p].action = action;
        table[state][p].target = target;
    }
}

// Fill a default rule into the table.
void fillDefault(
    entry table[][BIG], char *tokens[], char *states[], char *patterns[]
) {
    byte action = (byte) tokens[2][0];
    byte state = (byte) find(tokens[0], states);
    byte target = (byte) find(tokens[1], states);
    for (int p = 0; p < length(patterns); p++) {
        if (strlen(patterns[p]) != 0) continue;
        table[state][p].action = action;
        table[state][p].target = target;
    }
}

// Check for any missing defaults, and complain.
void checkMissing(
    entry table[][BIG], char *states[], char *patterns[]
) {
    for (int s = 0; s < length(states); s++) {
        for (int p = 0; p < length(patterns); p++) {
            if (table[s][p].action != SKIP) continue;
            if (strlen(patterns[p]) != 0) continue;
            crash("Default rule needed for state", 0, states[s]);
        }
    }
}

// Enter rules into the table.
void fillTable(
    entry table[][BIG], int n, char *tokens[n][SMALL],
    char *states[], char *patterns[]
) {
    for (int s=0; s<SMALL; s++) for (int p=0; p<BIG; p++) {
        table[s][p].action = SKIP;
    }
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        if (t == 3) fillDefault(table, tokens[i], states, patterns);
        else fillRule(table, tokens[i], states, patterns);
    }
    checkMissing(table, states, patterns);
}

scanner *buildScanner(char const *path) {
    scanner *sc = calloc(1, sizeof(scanner));
    readFile(path, sc->text);
    splitLines(sc->text, sc->lines);
    makeSingles();
    splitTokens(sc->lines, sc->tokens);
    int nlines = length(sc->lines);
    gatherStates(nlines, sc->tokens, sc->states);
    gatherPatterns(nlines, sc->tokens, sc->patterns);
    sort(sc->patterns);
    expandPatterns(sc->patterns, sc->temp);
    sc->nstates = length(sc->states);
    sc->npatterns = length(sc->patterns);
    fillTable(sc->table, nlines, sc->tokens, sc->states, sc->patterns);
    return sc;
}

void writeScanner(scanner *sc, char const *path) {
    FILE *fp = fopen(path, "w");
    char *states =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int col = 0;
    for (int i = 0; i < sc->nstates; i++) {
        char *s = sc->states[i];
        if (col + 3 + strlen(s) > 80) { fprintf(fp, "\n"); col = 0; }
        if (col > 0) fprintf(fp, " ");
        fprintf(fp, "%c=%s", states[i], s);
        col += 3 + strlen(s);
    }
    fprintf(fp, "\n");
    fprintf(fp, "\n");
    for (int s = 0; s < sc->nstates; s++) {
        fprintf(fp, "%c  ", states[s]);
    }
    fprintf(fp, "\n");
    for (int p = 0; p < sc->npatterns; p++) {
        for (int s = 0; s < sc->nstates; s++) {
            char action = (char) sc->table[s][p].action;
            char target = states[sc->table[s][p].target];
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
        {"s", "a", "b", "c", "t", "MORE", NULL},
        {"s", "\\s", "\\b", "t", "MORE", NULL},
        {"s", "a", "b", "c", "t", "MORE", NULL},
        {"s", "a", "t", "X", NULL},
    };
    char *tokens[4][SMALL] = {{NULL},{NULL},{NULL},{NULL}};
    makeSingles();
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
    char *patterns[10] = {NULL};
    char *expect[] = {"x", "y", NULL};
    gatherPatterns(2, ts, patterns);
    assert(eq(patterns, expect));
}

void testSort() {
    char *patterns[] = { "", "<", "<=", "!", NULL };
    char *expect[] = {"!", "<=", "<", "", NULL};
    sort(patterns);
    assert(eq(patterns, expect));
}

void testExpandPatterns() {
    char *patterns[] = { "!", "<=", "<", NULL, NULL, NULL, NULL, NULL };
    char *temp[10];
    char *expect[] = {"", "!", "", "<=", "<", "", NULL};
    expandPatterns(patterns, temp);
    assert(eq(patterns, expect));
}

int main(int n, char const *args[n]) {
    testSplitLines();
    testSplitTokens();
    testGatherStates();
    testGatherPatterns();
    testSort();
    testExpandPatterns();
    if (n != 2) crash("Use: ./compile language", 0, "");
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    scanner *sc = buildScanner(path);
    sprintf(path, "%s/table.txt", args[1]);
    writeScanner(sc, path);
    free(sc);
    return 0;
}
