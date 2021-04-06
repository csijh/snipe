// Snipe language interpreter. Free and open source. See license.txt.
// Read in a compiled language description and execute it for testing. Type
//
//     ./interpret [-t] c
//
// to read in c.bin and execute tests from src/c.txt where -t means trace.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

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

// A scanner consists of a table[nstates][npatterns], an array of state names,
// an array of pattern strings, and an array of token type names. The starters
// array gives the first pattern starting with each character.
struct scanner {
    short nstates, npatterns, ntypes;
    entry *table;
    char **states;
    char **patterns;
    char **types;
    short starters[128];
};
typedef struct scanner scanner;

void freeScanner(scanner *sc) {
    free(sc->table);
    free(sc->states);
    free(sc->patterns);
    free(sc->types);
    free(sc);
}

// Crash with a message.
void crash(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

// Read in a text file. Use binary mode, so that the number of bytes read
// equals the file size.
char *readTests(char *path) {
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

// Read in the scanner from a binary file.
void readScanner(char const *path, scanner *sc) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("Error: can't read file");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("Error: can't find file size");
    byte sizes[6];
    int n = fread(sizes, 1, 6, file);
    if (n != 6) crash("Error: reading file");
    sc->nstates = (sizes[0] << 8) | sizes[1];
    sc->npatterns = (sizes[2] << 8) | sizes[3];
    sc->ntypes = (sizes[4] << 8) | sizes[5];
    printf("ns=%d, np=%d, nt=%d\n", sc->nstates, sc->npatterns, sc->ntypes);
    size = size - 6;
    sc->table = malloc(size);
    n = fread(sc->table, 1, size, file);
    if (n != size) crash("Error: reading file");
//    int tableSize = sc->nstates * sc->npatterns * sizeof(entry);
    fclose(file);
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

int main(int n, char const *args[n]) {
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
    return 0;
}
