// Snipe language interpreter. Free and open source. See license.txt.
// Read in a compiled language description and execute it for testing. Type
//
//     ./interpret [-t] c
//
// to read in c.bin and execute tests from c-test.txt
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Special action constants. The rest are token tags.
typedef unsigned char byte;
enum { SKIP = '\0', ACCEPT = ' ', REJECT = '\r' };

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
    if (n != size) { free(data); fclose(file); crash("read failed"); }
    data[n] = '\0';
    fclose(file);
    return data;
}

// Split the text of a language description into an array of lines, replacing
// each newline by a null. Skip lines up to a line of at least 5 minuses, and
// blank lines.
char **splitLines(char *fullText) {
    char *text = strstr(fullText, "-----");
    if (text == NULL) crash("Error: no line of minuses");
    while (*text != '\n') text++;
    text++;
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

// Analyse the scanner.
void analyse(scanner *sc) {
    byte (*table)[sc->npatterns][2] = (byte(*)[sc->npatterns][2]) sc->table;
    for (char ch = ' '; ch <= '~'; ch++) {
        int target1 = -1;
        bool done = false;
        for (int s = 0; s < sc->nstates && ! done; s++) {
            for (int p = 0; p < sc->npatterns && ! done; p++) {
                if (table[s][p][0] != ch) continue;
                byte target = table[s][p][1];
                if (target1 < 0) target1 = target;
                else if (target != target1) {
                    printf("Except %c %d %d\n", ch, target1, target);
                    done = true;
                }
            }
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

// Scan a line, tagging the first byte of each token in the tokens array. Make a
// local table variable of effective type byte[h][w][2] even though h and w are
// dynamic.
void scan(char *line, char *tokens, scanner *sc, bool trace) {
    byte (*table)[sc->npatterns][2] = (byte(*)[sc->npatterns][2]) sc->table;
    int state = 0, start = 0, i = 0;
    while (i < strlen(line) || start < i) {
        int ch = line[i], action, len, target, p;
        for (p = sc->starters[ch]; ; p++) {
            if (p == 294) {
                printf("ch=%c, p0=%d, s='%s'\n", ch, sc->starters[ch], sc->patterns[196]);
                printf("ps=%d\n", sc->npatterns);
            }
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
        else if (action == REJECT) {
            i = start;
            while (start > 0 && tokens[start] == ' ') start--;
        }
        else {
            i += len;
            if (i > start) tokens[start] = action;
            start = i;
        }
        state = target;
    }
}

// Run the tests from the end of the language description file.
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
        "to read lang.bin and run tests from lang.txt");
    scanner *sc = malloc(sizeof(scanner));
    char path[100];
    sprintf(path, "%s.bin", lang);
    readScanner(path, sc);
    construct(sc);
    bool analysing = false;
    if (analysing) analyse(sc);
    sprintf(path, "%s.txt", lang);
    char *testFile = readTests(path);
    char **lines = splitLines(testFile);
    int p = runTests(lines, sc, trace);
    free(lines);
    free(testFile);
    freeScanner(sc);
    printf("Tests passed: %d\n", p);
    return 0;
}
