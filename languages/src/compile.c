// Snipe language compiler. Free and open source. See license.txt.

// Compile a language description into a scanner in a binary file. The program
// interpret.c can be used for testing. Scanning produces a tag for each byte in
// the text of a document. For the first byte of a token, the tag is the type of
// token. For subsequent bytes, the tag indicates a continuation of the token,
// with the grapheme boundaries marked. For spaces and newlines, the tag
// indicates the scanner state, to enable incremental re-scanning.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// BIG is the limit on array sizes. It can be increased as necessary.
// SMALL is the limit on arrays indexed by bytes.
enum { BIG = 10000, SMALL = 256 };

// A tag is an action in the transition table, or a label for a byte of the
// text. It may be the index of a token type, to indicate the action of
// terminating a token, and as a label for its first byte. As an action, it may
// be Skip to indicate that the entry is not relevant to the current state, or
// Accept to indicate the action of continuing the same token. As a label, it
// may be Byte to label a continuation byte within a grapheme, or Char to label
// a continuation character (grapheme) within a token, or (for a space or
// newline) the index number of a state to enable re-scanning from that point.
enum { Skip = 0, Accept = 1, Byte = 0, Char = 1 };

// Each table entry contains a tag representing an action, and a target state.
typedef unsigned char byte;
struct entry { byte action, target; };
typedef struct entry entry;

// A scanner structure consists of the number of states and patterns (which
// determines the size of the state transition table) and the number of token
// types, then the table itself, then a string store containing the state names
// followed by the patterns followed by the token type names. The remaining
// fields are for temporary use during construction. Many are NULL terminated
// arrays of strings.
struct scanner {
    short nstates, npatterns, ntypes;
    entry table[SMALL][BIG];
    char strings[BIG];

    char text[BIG];
    char *lines[BIG];
    char *tokens[BIG][SMALL];
    char *states[SMALL];
    char *patterns[BIG];
    char *types[SMALL];
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
// other control characters.
void validate(int n, char *line) {
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
        validate(n, &text[p]);
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

// Split each line into an array of tokens, replacing spaces by null characters
// as necessary. Expand ranges into explicit one-character tokens. If there is
// no token type, add a " " token.
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
        char *last = tokens[i][length(tokens[i])-1];
        if ('a' <= last[0] && last[0] <= 'z') {
            add(singles[' '], tokens[i]);
        } else {
            if (last[0] < 'A' || last[0] > 'Z') {
                crash("Expecting upper case token type", i, last);
            }
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

// Gather token types, with types[Skip/Byte] = "." and types[Accept/Char] = " ".
void gatherTypes(int n, char *tokens[n][SMALL], char *types[]) {
    find(".", types);
    find(" ", types);
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]) - 1;
        find(tokens[i][t], types);
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
    entry table[][BIG], char *tokens[], char *states[], char *patterns[],
    char *types[]
) {
    int n = length(tokens);
    byte action = (byte) find(tokens[n-1], types);
    byte state = (byte) find(tokens[0], states);
    byte target = (byte) find(tokens[n-2], states);
    for (int i = 1; i < n-1; i++) {
        short p = (short) find(tokens[i], patterns);
        byte oldAction = table[state][p].action;
        if (oldAction != Skip) continue;
        table[state][p].action = action;
        table[state][p].target = target;
    }
}

// Fill a default rule into the table.
void fillDefault(
    entry table[][BIG], char *tokens[], char *states[], char *patterns[],
    char *types[]
) {
    byte action = (byte) find(tokens[2], types);
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
            if (table[s][p].action != Skip) continue;
            if (strlen(patterns[p]) != 0) continue;
            crash("Default rule needed for state", 0, states[s]);
        }
    }
}

// Enter rules into the table.
void fillTable(
    entry table[][BIG], int n, char *tokens[n][SMALL],
    char *states[], char *patterns[], char *types[]
) {
    for (int s=0; s<SMALL; s++) for (int p=0; p<BIG; p++) {
        table[s][p].action = Skip;
    }
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        if (t == 3) fillDefault(table, tokens[i], states, patterns, types);
        else fillRule(table, tokens[i], states, patterns, types);
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
    gatherTypes(nlines, sc->tokens, sc->types);
    sort(sc->patterns);
    expandPatterns(sc->patterns, sc->temp);
    sc->nstrings = transfer(sc->states, sc->nstrings, sc->strings);
    sc->nstrings = transfer(sc->patterns, sc->nstrings, sc->strings);
    sc->nstrings = transfer(sc->types, sc->nstrings, sc->strings);
    sc->nstates = length(sc->states);
    sc->npatterns = length(sc->patterns);
    sc->ntypes = length(sc->types);
    fillTable(
        sc->table, nlines, sc->tokens, sc->states, sc->patterns, sc->types);
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
    writeShort(sc->ntypes, fp);
    for (int s = 0; s < sc->nstates; s++) {
        for (int p = 0; p < sc->npatterns; p++) {
            fwrite(&sc->table[s][p].action, 1, 1, fp);
            fwrite(&sc->table[s][p].target, 1, 1, fp);
        }
    }
    fwrite(sc->strings, 1, sc->nstrings, fp);
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
    char *lines[4] = {s1, s2, s3, NULL};
    char *expect[3][7] = {
        {"s", "a", "b", "c", "t", " ", NULL},
        {"s", "\\s", "\\b", "t", " ", NULL},
        {"s", "a", "b", "c", "t", " ", NULL}
    };
    char *tokens[3][SMALL] = {{NULL},{NULL},{NULL}};
    makeSingles();
    splitTokens(lines, tokens);
    assert(eq(tokens[0], expect[0]));
    assert(eq(tokens[1], expect[1]));
    assert(eq(tokens[2], expect[2]));
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

void testGatherTypes() {
    char *ts[2][SMALL] = {{"s","x","s","X",NULL}, {"s","y","s","Y",NULL}};
    char *types[10] = {NULL};
    char *expect[] = {".", " ", "X", "Y", NULL};
    gatherTypes(2, ts, types);
    assert(eq(types, expect));
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
    testGatherTypes();
    testSort();
    testExpandPatterns();
    if (n != 2) crash("Use: ./compile language", 0, "");
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    scanner *sc = buildScanner(path);
    sprintf(path, "%s/lang.bin", args[1]);
    writeScanner(sc, path);
    free(sc);
    return 0;
}
