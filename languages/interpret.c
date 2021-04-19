// Snipe language interpreter. Free and open source. See licence.txt.
// Read in a compiled language description and execute it for testing. Type
//
//     ./interpret [-t] c
//
// to read in c/table.bin and execute tests c/tests.txt where -t means trace.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Big is the limit on the number of patterns. Small is the limit on other
// arrays, allowing them to be indexed by char.
enum { BIG = 1000, SMALL = 128 };

// The state machine transition table contains actions consisting of a tag and
// state index.
struct action { char tag, target; };
typedef struct action action;

// A tag represents a token type, or SKIP to miss out that table entry or MORE
// to add to the current token. Each byte of text read in is labelled with a tag
// which represents the token type, or SKIP for a continuation byte of the
// current character (or grapheme) or MORE for a continuation character of the
// current token. Spaces are tagged with GAP, and newlines with NEWLINE.
enum { SKIP = '~', MORE = '-', GAP = '_', NEWLINE = '.' };

// A scanner consists of a current state, current input position, current
// position in the tags, current start of token position in the tags, the array
// of state names, the array of actions for each state, the array of indexes of
// the patterns starting with each character, pointers to the pattern strings
// starting with each character, and a reference to the whole of the data read
// in from the file.
struct scanner {
    char state;
    char *input;
    char *tags;
    char *token;
    char *states[SMALL];
    action *actions[SMALL];
    short indexes[SMALL];
    char *patterns[BIG];
    char *data;
};
typedef struct scanner scanner;

// Crash with a message.
void crash(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

// ---------- Read in data -----------------------------------------------------

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

// Read the scanner from a file. The file contains the names of the states, each
// name being a string preceded by a length byte, then a zero byte, then the
// pattern strings, each preceded by a length byte with the top bit set for a
// lookahead pattern, then a zero byte, then a table of actions indexed by state
// and pattern.
void readScanner(char *path, scanner *sc) {
    char *d = readFile(path);
    sc->data = d;
    int p = 0, nstates = 0, npatterns = 0;
    while (d[p] != 0) {
        int len = d[p++];
        sc->states[nstates++] = &d[p];
        p = p + len + 1;
    }
    p++;
    while (d[p] != 0) {
        int len = (d[p] & 0x7F);
        sc->patterns[npatterns++] = &d[p];
        p = p + len + 2;
    }
    p++;
    int i = 0;
    for (int ch = 0; ch < SMALL; ch++) {
        sc->indexes[ch] = i;
        while (i < npatterns && sc->patterns[i][1] == ch) i++;
    }
    for (int s = 0; s < nstates; s++) {
        sc->actions[s] = (action *) &d[p + s * npatterns * sizeof(action)];
    }
    printf("#states=%d #patterns=%d\n", nstates, npatterns);
//    for (int i = 0; i <= 181; i++) {
//        printf("%c%d\n", sc->actions[0][i].tag, sc->actions[0][i].target);
//    }
}

// --------- Scan --------------------------------------------------------------

// Match a string with a pattern. Return the length of the pattern, or 0 for a
// lookahead pattern, or -1 if there is no match.
static inline int match(char *s, char *p) {
    int n = p[0] & 0x7F;
    bool lookahead = (p[0] & 0x80) != 0;
    p++;
    for (int i = 0; i < n; i++) if (s[i] != p[i]) return -1;
    if (lookahead) return 0;
    return n;
}

// Print out a trace of a scan step, in the style of the original rules, with
// \n and \s for newline and space.
void trace(scanner *sc, int base, char *p, int target, char tag) {
    bool lookahead = (p[0] & 0x80) != 0;
    if (lookahead) printf("%c ", tag);
    printf("%s ", sc->states[base]);
    if (p[1] == '\n') printf("\\n ");
    else if (p[1] == ' ') printf("\\s ");
    else printf("%s ", (p+1));
    printf("%s", sc->states[target]);
    if (! lookahead) printf(" %c", tag);
    printf("\n");
}

// Given a state and the current position in the input, find the pattern that
// matches (which should never fail) and mark the matched characters with MORE.
// Update the state and input position.
void step(scanner *sc, bool tracing) {
    int s = sc->state;
    action *actions = sc->actions[s];
    int ch = sc->input[0];
    int index = sc->indexes[ch];
    int m = -1;
    char *p;
    while (m < 0) {
        p = sc->patterns[index];
        if (actions[index].tag != SKIP) m = match(sc->input, p);
        if (m < 0) index++;
    }
    for (int i = 0; i < m; i++) {
        *sc->tags++ = MORE;
        sc->input++;
    }
    char tag = actions[index].tag;
    char target = actions[index].target;
    if (tracing) trace(sc, sc->state, p, target, tag);
    if (tag != MORE) {
        sc->token[0] = tag;
        sc->token = sc->tags;
    }
    sc->state = target;
}

// Scan a line, tagging each byte in the tags array.
void scan(scanner *sc, char *line, char *tags, bool tracing) {
    sc->state = 0;
    sc->input = line;
    sc->tags = tags;
    sc->token = tags;
    while (sc->input[0] != '\0') step(sc, tracing);
}

/*
    int state = 0, start, i = 0, lineLength = strlen(line);
    while (line[i] == ' ') tags[i++] = GAP;
    start = i;
    while (i < lineLength || start < i) {
        printf("i %d start %d\n", i, start);
        int ch = line[i], action, len, target = -1, p, lookingahead = false;
        if (ch == ' ') {
            while (line[i] == ' ') tags[i++] = GAP;
            lookingahead = true;
        }
        for (p = sc->starters[ch]; sc->patterns[p][0] == ch; p++) {
            action = sc->table[state][p].action;
            if (action == SKIP) continue;
            len = match(&line[i], sc->patterns[p]);
            if (len == 0) continue;
            if (lookingahead && ! sc->lookahead[p]) continue;
            if (sc->lookahead[p]) lookingahead = true;
            target = sc->table[state][p].target;
            break;
        }
        if (target < 0) {
            p = sc->npatterns-1;
            action = sc->table[state][p].action;
            target = sc->table[state][p].target;
        }
        if (trace) {
            printf("%d %s '%s' %c\n", i, sc->states[state], sc->patterns[p], action);
        }
        if (! lookingahead) {
            for (int j = 0; j<len; j++) tags[i++] = MORE;
        }
        if (action != MORE) {
            if (i > start) tags[start] = action;
            start = i;
            while (line[i] == ' ') tags[i++] = GAP;
        }
        state = target;
    }
}
*/
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
    readScanner("c/table.bin", sc);
    char line[] = "abc\n";
    char tags[] = "???\n";
    scan(sc, line, tags, true);
    printf("tags %s\n", tags);
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
    free(sc->data);
    free(sc);
    return 0;
}
