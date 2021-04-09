// Snipe language interpreter. Free and open source. See licence.txt.
// Read in a compiled language description and execute it for testing. Type
//
//     ./interpret [-t] c
//
// to read in c/table.txt and execute tests c/tests.txt where -t means trace.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// BIG is the limit on array sizes. It can be increased as necessary.
// SMALL is the limit on arrays indexed by unsigned bytes.
enum { BIG = 10000, SMALL = 256 };

// In the transition table, an action is a tag which represents a token type, or
// SKIP to miss out that table entry or MORE to add to the current token. Each
// byte of text read in is labelled with a tag which represents the token type,
// or SKIP for a continuation byte of the current character (or grapheme) or
// MORE for a continuation character of the current token. Spaces are tagged
// with GAP, and newlines with NEWLINE.
typedef unsigned char byte;
struct entry { byte action, target; };
typedef struct entry entry;
enum { SKIP = '~', MORE = '-', GAP = '_', NEWLINE = '.' };

// A scanner consists of a table[nstates][npatterns], an array of state names,
// and an array of pattern strings. The starters array gives the first pattern
// starting with each character.
struct scanner {
    short nstates, npatterns;
    entry table[SMALL][BIG];
    char *states[SMALL];
    char *patterns[BIG];
    short starters[128];
};
typedef struct scanner scanner;

// Crash with a message.
void crash(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

// Read in a text file. Use binary mode, so that the number of bytes read
// equals the file size.
char *readFile(char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("Error: can't read file");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("Error: can't find file size");
    char *data = malloc(size + 1);
    int n = fread(data, 1, size, file);
    if (n != size) { free(data); fclose(file); crash("Error: read failed"); }
    data[n] = '\0';
    fclose(file);
    return data;
}

static char *stateLabels =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Split a line into an array of tokens, replacing spaces by null characters
// as necessary, and return the number of tokens.
int splitTokens(char *line, char *tokens[SMALL]) {
    int ntokens = 0;
    int start = 0, end = 0, len = strlen(line);
    while (line[start] == ' ') start++;
    while (start < len) {
        end = start;
        while (line[end] != ' ' && line[end] != '\0') end++;
        line[end] = '\0';
        char *token = &line[start];
        tokens[ntokens++] = token;
        start = end + 1;
        while (start < len && line[start] == ' ') start++;
    }
    return ntokens;
}


void readStateNames(char *s, scanner *sc) {
    sc->nstates = splitTokens(s, sc->states);
    for (int i = 0; i < sc->nstates; i++) {
        char *state = sc->states[i];
        if (state[0] != stateLabels[i]) crash("Error: bad table");
        if (state[1] != '=') crash("Error: bad table");
        sc->states[i] = sc->states[i] + 2;
    }
}

// Check header and return rest of table.
char *readHeader(char *data, scanner *sc) {
    char *tokens[SMALL];
    char *s = strstr(data, "\n");
    if (s == NULL) crash("Error: bad table");
    s[0] = '\0';
    int n = splitTokens(data, tokens);
    if (n != sc->nstates) crash("Error: bad table header");
    for (int i = 0; i < n; i++) {
        if (strlen(tokens[i]) != 1) crash("Error: bad table header");
        if (tokens[i][0] != stateLabels[i]) crash("Error: bad table header");
    }
    return s + 1;
}

// Read one row of the table.
void readRow(int row, char *line, scanner *sc) {
    char *tokens[SMALL];
    int n = splitTokens(line, tokens);
    if (n != sc->nstates + 1) crash("Error bad table row");
    for (int i = 0; i < n-1; i++) {
        sc->table[i][row].action = tokens[i][0];
        char *s = strchr(stateLabels, tokens[i][1]);
        sc->table[i][row].target = s - stateLabels;
    }
    if (strcmp(tokens[n-1], "default") == 0) sc->patterns[row] = "";
    else sc->patterns[row] = tokens[n-1];
}

// Read the rows of the table.
void readTable(char *data, scanner *sc) {
    int p = 0, n = 0;
    for (int i = 0; data[i] != '\0'; i++) {
        if (data[i] != '\n') continue;
        data[i]= '\0';
        readRow(n, &data[p], sc);
        n++;
        p = i + 1;
    }
    sc->npatterns = n;
    printf("rows %d\n", n);
}

// Read the state names and then the table from table.txt.
void readScanner(char *data, scanner *sc) {
    char *s = strstr(data, "\n\n");
    if (s == NULL) crash("Error: no blank line");
    s[0] = '\0';
    for (int i = 0; i < strlen(data); i++) if (data[i] == '\n') data[i] = ' ';
    readStateNames(data, sc);
    data = s + 2;
    data = readHeader(data, sc);
    readTable(data, sc);
}

/*
// Split the text of a test file into an array of lines, replacing each newline
// by a null. Skip blank lines.
char **splitLines(char *text) {
    int n = 0;
    for (int i = 0; text[i] != '\0'; i++) if (text[i] == '\n') n++;
    char **lines = malloc((n+1) * sizeof(char *));
    int p = 0;
    n = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\r') text[i] = '\0';
        if (text[i] != '\n') continue;
        text[i]= '\0';
        if (text[p] != '\0') lines[n++] = &text[p];
        p = i + 1;
    }
    lines[n] = NULL;
    return lines;
}

// Build the scanner from the data read in.
void construct(scanner *sc) {
    sc->states = malloc(sc->nstates * sizeof(char *));
    sc->patterns = malloc(sc->npatterns * sizeof(char *));
    sc->types = malloc(sc->ntypes * sizeof(char *));
    int tableSize = sc->nstates * sc->npatterns * sizeof(entry);
    char *strings = (char *)(sc->table) + tableSize;
    for (int i = 0; i < sc->nstates; i++) {
        sc->states[i] = strings;
        strings += strlen(strings) + 1;
    }
    for (int i = 0; i < sc->npatterns; i++) {
        sc->patterns[i] = strings;
        strings += strlen(strings) + 1;
    }
    for (int i = 0; i < sc->ntypes; i++) {
        sc->types[i] = strings;
        strings += strlen(strings) + 1;
    }
    int p = 0;
    for (int i = 0; i < 128; i++) {
        if (p >= sc->npatterns - 1 || i < sc->patterns[p+1][0]) {
            sc->starters[i] = p;
        }
        else {
            p++;
            sc->starters[i] = p;
            while (sc->patterns[p][0] != '\0') p++;
        }
    }
}

// Check if a string starts with a pattern. Return the length or -1.
static inline int match(char *s, char *p) {
    int i;
    for (i = 0; p[i] != '\0'; i++) {
        if (s[i] != p[i]) return -1;
    }
    return i;
}

// Scan a line, tagging each byte in the tags array. Make a local table variable
// of effective type entry[h][w] even though h and w are dynamic.
void scan(char *line, char *tags, scanner *sc, bool trace) {
    entry (*table)[sc->npatterns] = (entry(*)[sc->npatterns]) sc->table;
    int state = 0, start = 0, i = 0;
    while (line[i] == ' ') i++;
    while (i < strlen(line) || start < i) {
        int ch = line[i], action, len, target, p;
        for (p = sc->starters[ch]; ; p++) {
            action = table[state][p].action;
            target = table[state][p].target;
            if (action == Skip) continue;
            len = match(&line[i], sc->patterns[p]);
            if (len >= 0) break;
        }
        if (trace) {
            printf(
                "%d %s '%s' %s\n",
                i, sc->states[state], sc->patterns[p], sc->types[action]
            );
        }
        if (action == Accept) i += len;
        else {
            i += len;
            if (i > start) tags[start] = sc->types[action][0];
            start = i;
            while (line[i] == ' ') {
                if (i > start) tags[start] = sc->types[Char][0];
                i++;
                start = i;
            }
        }
        state = target;
    }
}

// Run the tests from the test file.
int runTests(char**lines, scanner *sc, bool trace) {
    char tags[100];
    int passes = 0;
    for (int i = 0; lines[i] != NULL; i = i + 2) {
        for (int i = 0; i < 100; i++) tags[i] = ' ';
        tags[strlen(lines[i])] = '\0';
        scan(lines[i], tags, sc, trace);
        int n = strlen(tags);
        while (n > 0 && tags[n-1] == ' ') tags[--n] = '\0';
        if (strcmp(tags, lines[i+1]) == 0) { passes++; continue; }
        fprintf(stderr, "Error:\n");
        fprintf(stderr, "%s\n", lines[i]);
        fprintf(stderr, "%s\n", lines[i+1]);
        fprintf(stderr, "%s\n", tags);
        exit(1);
    }
    return passes;
}
*/
int main(int n, char const *args[n]) {
    scanner *sc = malloc(sizeof(scanner));
    char *data = readFile("c/table.txt");
    readScanner(data, sc);

    for (int i = 0; i < sc->nstates; i++) {
        printf("%d %s\n", i, sc->states[i]);
    }
    /*
    char const *lang;
    bool trace = n > 2;
    if (n == 2) lang = args[1];
    else if (n == 3 && strcmp(args[1], "-t") == 0) lang = args[2];
    else if (n == 3 && strcmp(args[2], "-t") == 0) lang = args[1];
    else crash("Use: ./interpret [-t] lang\n"
        "to read lang.bin and run tests from tests/lang.txt");
    scanner *sc = malloc(sizeof(scanner));
    char path[100];
    sprintf(path, "%s/lang.bin", lang);
    readScanner(path, sc);
    construct(sc);
    sprintf(path, "%s/tests.txt", lang);
    char *testFile = readTests(path);
    char **lines = splitLines(testFile);
    int p = runTests(lines, sc, trace);
    free(lines);
    free(testFile);
    freeScanner(sc);
    printf("Tests passed: %d\n", p);
    */
    free(sc);
    free(data);
    return 0;
}
