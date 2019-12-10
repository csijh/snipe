// Snipe language compiler. Free and open source. See license.txt.
// Compile a language description into a scanner in a binary file. The program
// interpret.c can be used for testing.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// BIG is the limit on array sizes. It can be increased as necessary.
// SMALL is the limit on arrays indexed by bytes.
enum { BIG = 10000, SMALL = 256 };

// A table entry consists of two bytes, an action and a target state. An action
// is an ASCII character. SKIP is reserved to mean that this pattern is not
// relevant in the current state, accept means add a character to the current
// token, and reject means backtrack to the start of the current token.
typedef unsigned char byte;
enum { SKIP = '\0', ACCEPT = ' ', REJECT = '\r' };

// A scanner structure consists of the size of the state transition table then
// the table itself, then a string store containing the state names followed by
// the patterns. The remaining fields are for temporary use during construction.
// Many are NULL terminated arrays of strings.
struct scanner {
    short nstates, npatterns;
    byte table[SMALL][BIG][2];
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

// Crash with an error message and possibly a line number or file name.
void crash(char *e, int n, char const *s) {
    fprintf(stderr, "Error");
    if (n > 0) fprintf(stderr, " on line %d", n);
    fprintf(stderr, ": %s\n", e);
    if (strlen(s) > 0) fprintf(stderr, " %s", s);
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
// other control characters except '\n'. Check that an initial state name has at
// least two characters. Check that the only escapes are \s,\b,\n,\r and a
// backslash followed by whitespace.
void validate(int n, char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        unsigned char ch = line[i];
        if (ch == '\t' || ch == '\r') line[i] = ' ';
        else if (ch >= 128) crash("non-ASCII character", n, "");
        else if (ch < 32 || ch > 126) crash("control character", n, "");
        else if (ch == '\\') {
            ch = line[i+1];
            if (strchr("sbnr \n", ch) == NULL) {
                crash("unknown escape sequence", n, "");
            }
        }
    }
    if (isalpha(line[0]) && (line[1] == ' ' || line[1] == '\0')) {
        crash("state name too short", n, "");
    }
}

// Split the text into an array of lines, replacing each newline by a null. Skip
// blank lines or lines starting with a non-letter. Stop at a line starting with
// at least 5 minus signs.
void splitLines(char *text, char *lines[]) {
    int p = 0, n = 1;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\r') text[i] = ' ';
        if (text[i] != '\n') continue;
        text[i]= '\0';
        if (strncmp(&text[p], "-----", 3) == 0) break;
        validate(n, &text[p]);
        if (text[p] != '\0' && isalpha(text[p])) {
            add(&text[p], lines);
        }
        n++;
        p = i + 1;
    }
}

// Interpret escape sequences.
void unescape(char *s) {
    int n = 0, len = strlen(s);
    for (int i = 0; i < len; i++) {
        if (s[i] == '\\' && i < len-1) {
            i++;
            if (s[i] == 's') s[n++] = ' ';
            else if (s[i] == 'b') s[n++] = '\\';
            else if (s[i] == 'n') s[n++] = '\n';
            else if (s[i] == 'r') s[n++] = '\r';
        }
        else s[n++] = s[i];
    }
    s[n] = '\0';
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

// Split each line into an array of tokens, replacing spaces by null characters
// as necessary. Expand ranges into explicit one-character tokens. Add a missing
// accept action.
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
            unescape(pattern);
            if (isRange(pattern)) expandRange(pattern, tokens[i]);
            else add(pattern, tokens[i]);
            start = end + 1;
            while (start < len && line[start] == ' ') start++;
        }
        char *last = tokens[i][length(tokens[i])-1];
        if (strlen(last) > 1) {
            add(singles[ACCEPT], tokens[i]);
        }
    }
}

// Gather distinct state names from the tokens. The position of the name in
// the array is the state number.
void gatherStates(int n, char *tokens[n][SMALL], char *states[]) {
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        find(tokens[i][0], states);
        find(tokens[i][t-2], states);
    }
}

// Gather pattern strings.
void gatherPatterns(int n, char *tokens[n][SMALL], char *patterns[]) {
    for (int i = 0; i < n; i++) {
        for (int t = 1; t < length(tokens[i]) - 2; t++) {
            find(tokens[i][t], patterns);
        }
    }
}

// ----- Sorting --------------------------------------------------------------

// Check if string s is a prefix of string t.
bool prefix(char const *s, char const *t) {
    return strncmp(s, t, strlen(s)) == 0;
}

// Compare two strings in ascii order, except prefer longer strings.
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

// Transfer a list to the strings array, returning the updated length.
int transfer(char *list[], int n, char strings[n]) {
    for (int i = 0; i < length(list); i++) {
        strcat(&strings[n], list[i]);
        list[i] = &strings[n];
        n += strlen(list[i]) + 1;
    }
    return n;
}

// Fill a non-default rule into the table.
void fillRule(
    byte table[][BIG][2], char *tokens[], char *states[], char *patterns[]
) {
    int n = length(tokens);
    byte action = tokens[n-1][0];
    byte state = (byte) find(tokens[0], states);
    byte target = (byte) find(tokens[n-2], states);
    for (int i = 1; i < n-1; i++) {
        short p = (short) find(tokens[i], patterns);
        byte oldAction = table[state][p][0];
        if (oldAction != SKIP) continue;
        table[state][p][0] = action;
        table[state][p][1] = target;
    }
}

// Fill a default rule into the table.
void fillDefault(
    byte table[][BIG][2], char *tokens[], char *states[], char *patterns[]
) {
    byte action = tokens[2][0];
    byte state = (byte) find(tokens[0], states);
    byte target = (byte) find(tokens[1], states);
    for (int p = 0; p < length(patterns); p++) {
        char *pattern = patterns[p];
        if (strlen(pattern) != 0) continue;
        if (p == 180) printf("fill %s %d %d\n", pattern, action, target);
        table[state][p][0] = action;
        table[state][p][1] = target;
    }
}

// Enter rules into the table.
void fillTable(
    byte table[][BIG][2], int n, char *tokens[n][SMALL],
    char *states[], char *patterns[]
) {
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        if (t == 3) fillDefault(table, tokens[i], states, patterns);
        else fillRule(table, tokens[i], states, patterns);
    }
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
    sc->nstrings = transfer(sc->states, sc->nstrings, sc->strings);
    sc->nstrings = transfer(sc->patterns, sc->nstrings, sc->strings);
    sc->nstates = length(sc->states);
    sc->npatterns = length(sc->patterns);
    fillTable(sc->table, nlines, sc->tokens, sc->states, sc->patterns);
    return sc;
}

// Write a short to a file in big endian order.
void writeShort(short n, FILE *fp) {
    byte bytes[2] = { n >> 8, n & 0xFF };
    fwrite(bytes, 1, 2, fp);
}

void writeScanner(scanner *sc, char const *path) {
    FILE *fp = fopen(path, "wb");
    writeShort(sc->nstates, fp);
    writeShort(sc->npatterns, fp);
    for (int s = 0; s < sc->nstates; s++) {
        for (int p = 0; p < sc->npatterns; p++) {
            fwrite(sc->table[s][p], 1, 2, fp);
        }
    }
    fwrite(sc->strings, 1, sc->nstrings, fp);
    fclose(fp);
}

// ----- Testing --------------------------------------------------------------

// Check two NULL terminated arrays of strings are equal.
bool eq(char *as[], char *bs[]) {
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
    char *lines[4] = {s1, s2, s3, NULL};
    char *expect[3][6] = {
        {"s", "a", "b", "c", "t", NULL},
        {"s", " ", "\\", "t", NULL},
        {"s", "a", "b", "c", "t", NULL}
    };
    char *tokens[3][SMALL] = {{NULL},{NULL},{NULL}};
    makeSingles();
    splitTokens(lines, tokens);
    assert(eq(tokens[0], expect[0]));
    assert(eq(tokens[1], expect[1]));
    assert(eq(tokens[2], expect[2]));
}

void testGatherStates() {
    char *ts[2][SMALL] = {{"s0","?","s1","?",NULL}, {"s0","s2","?",NULL}};
    char *states[10] = {NULL};
    char *expect[] = {"s0", "s1", "s2", NULL};
    gatherStates(2, ts, states);
    assert(eq(states, expect));
}

void testGatherPatterns() {
    char *ts[2][SMALL] = {{"s","x","s","?",NULL}, {"s","y","s","?",NULL}};
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
    if (n != 2) crash("Use: ./compile language.txt", 0, "");
    scanner *sc = buildScanner(args[1]);
    char out[100];
    strcpy(out, args[1]);
    n = strlen(out);
    strcpy(&out[n-3], "bin");
    writeScanner(sc, out);
    free(sc);
    return 0;
}
