// Snipe language interpreter. Free and open source. See license.txt.
// Read in a compiled language description and execute it for testing.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Special action constants. The rest are token tags.
typedef unsigned char byte;
enum { SKIP = '\0', ACCEPT = '.', REJECT = '!' };

// A scanner consists of a table[nstates][npatterns][2], an array of state
// names, and an array of pattern strings. The starters array gives the first
// pattern starting with each character.
struct scanner {
    short nstates, npatterns;
    byte *table;
    char **states;
    char **patterns;
    short starters[128];
};
typedef struct scanner scanner;

void freeScanner(scanner *sc) {
    free(sc->table);
    free(sc->states);
    free(sc->patterns);
    free(sc);
}

// Crash with a message.
void crash(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

// Read in the scanner from a binary file.
void readFile(char const *path, scanner *sc) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("Error: can't read file");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("Error: can't find file size");
    byte sizes[4];
    int n = fread(sizes, 1, 4, file);
    if (n != 4) crash("Error: reading file");
    sc->nstates = (sizes[0] << 8) | sizes[1];
    sc->npatterns = (sizes[2] << 8) | sizes[3];
    size = size - 4;
    sc->table = malloc(size);
    n = fread(sc->table, 1, size, file);
    if (n != size) crash("Error: reading file");
    fclose(file);
}

// Build the scanner from the data read in.
void construct(scanner *sc) {
    sc->states = malloc(sc->nstates * sizeof(char *));
    sc->patterns = malloc(sc->npatterns * sizeof(char *));
    char *strings = (char *)(sc->table + sc->nstates * sc->npatterns * 2);
    for (int i = 0; i < sc->nstates; i++) {
        sc->states[i] = strings;
        strings += strlen(strings) + 1;
    }
    for (int i = 0; i < sc->npatterns; i++) {
        sc->patterns[i] = strings;
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

void scan(char *line, char *tokens, scanner *sc, bool trace) {
    byte (*table)[sc->npatterns][2] = (byte(*)[sc->npatterns][2]) sc->table;
    int state = 0, start = 0;
    for (int i = 0; i < strlen(line) || start < i; ) {
        int ch = line[i], action, len, target, p;
        for (p = sc->starters[ch]; ; p++) {
            action = table[state][p][0];
            target = table[state][p][1];
            char c = action;
            if (c == 0) c = '-';
            if (action == SKIP) continue;
            len = match(&line[i], sc->patterns[p]);
            if (len >= 0) break;
        }
        if (trace) {
            printf(
                "%d %s '%s' %c\n",
                i, sc->states[state], sc->patterns[p], action
            );
        }
        if (action == ACCEPT) i += len;
        else if (action != REJECT) {
            tokens[start] = action;
            start = i;
            i += len;
        }
        state = target;
    }
}

int main(int n, char const *args[n]) {
    char const *path;
    bool trace;
    trace = n > 2;
    if (n == 2) path = args[1];
    else if (n == 3 && strcmp(args[1], "-t") == 0) path = args[2];
    else if (n == 3 && strcmp(args[2], "-t") == 0) path = args[1];
    else crash("Use: ./interpret [-t] language.bin");
    scanner *sc = malloc(sizeof(scanner));
    readFile(path, sc);
    construct(sc);
    char line[] = "abc";
    char tokens[] = "   ";
    scan(line, tokens, sc, trace);
    printf("%s\n%s\n", line, tokens);
    freeScanner(sc);
    return 0;
}
