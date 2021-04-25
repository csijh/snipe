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
#include <ctype.h>
#include <assert.h>

// Big is the limit on the number of patterns. Small is the limit on other
// arrays, allowing them to be indexed by char.
enum { BIG = 1000, SMALL = 128 };

// The state machine transition table contains actions consisting of a tag and
// state index.
struct action { unsigned char tag, target; };
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
// the patterns starting with each character, the array of pattern strings, and
// a reference to the whole of the data read in from the file.
struct scanner {
    int state;
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

// Read in a text or binary file. Add a final newline and null in case text.
char *readFile(char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("Error: can't read file");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("Error: can't find file size");
    char *data = malloc(size+2);
    int n = fread(data, 1, size, file);
    if (n != size) { free(data); fclose(file); crash("Error: read failed"); }
    data[n] = '\n';
    data[n+1] = '\0';
    fclose(file);
    return data;
}

// Read the transition table from a file to form a scanner. The file contains
// the names of the states, a null, the pattern strings, a null, and a table of
// actions indexed by state and pattern.
void readScanner(char *path, scanner *sc) {
    char *d = readFile(path);
    sc->data = d;
    int p = 0, nstates = 0, npatterns = 0;
    while (d[p] != 0) {
        sc->states[nstates++] = &d[p];
        while (d[p] != '\0') p++;
        p++;
    }
    p++;
    while (d[p] != 0) {
        sc->patterns[npatterns++] = &d[p];
        while (d[p] != '\0') p++;
        p++;
    }
    p++;
    int i = 0;
    for (int ch = 0; ch < SMALL; ch++) {
        sc->indexes[ch] = i;
        while (i < npatterns && sc->patterns[i][0] == ch) i++;
    }
    for (int s = 0; s < nstates; s++) {
        sc->actions[s] = (action *) &d[p + s * npatterns * sizeof(action)];
    }
    printf("%d states, %d patterns\n", nstates, npatterns);
}

// --------- Scan --------------------------------------------------------------

// Match a string with a pattern. Return the length of the pattern, or 0 if
// there is no match.
static inline int match(char *s, char *p) {
    int i;
//    printf("match %s %s\n", s, p);
    for (i = 0; p[i] != '\0'; i++) if (s[i] != p[i]) return 0;
    return i;
}

// Print out a trace of a scan step, in the style of the original rules, with
// \n and \s for newline and space.
static void trace(scanner *sc, int base, char *p, int target, char tag) {
    bool lookahead = (tag & 0x80) != 0;
    tag = tag & 0x7F;
    if (lookahead) printf("%c ", tag);
    printf("%s ", sc->states[base]);
    if (p[0] == '\n') printf("\\n ");
    else if (p[0] == ' ') printf("\\s ");
    else printf("%s ", p);
    printf("%s", sc->states[target]);
    if (! lookahead) printf(" %c", tag);
    printf("\n");
}

// Skip spaces and newlines. Search for a match, but ignore non-lookahead
// actions. Return the pattern index or -1 for a failed search (on finding
// a singleton pattern).
static int lookahead(scanner *sc) {
    action *actions = sc->actions[sc->state];
    char *input = sc->input;
    while (input[0] == ' ' || input[0] == '\n') input++;
    int ch = input[0];
    if (ch == '\0') return -1;
    int index = sc->indexes[ch];
    int len = 0;
    char *p;
    while (len == 0) {
        p = sc->patterns[index];
        int tag = actions[index].tag;
//        printf("la %d %s %d\n", index, p, tag);
        if (tag != SKIP && (tag & 0x80) != 0) len = match(input, p);
        if (len == 0) {
            if (p[1] == '\0') return -1;
            index++;
        }
    }
    return index;
}

// Do a normal search for a match (for a non-lookahead action, or a lookahead
// action with no intervening spaces - should never fail). If not a lookahead,
// move forward in the input. Return the pattern index.
static int search(scanner *sc) {
    action *actions = sc->actions[sc->state];
    int ch = sc->input[0];
    int index = sc->indexes[ch];
    int len = 0;
    char *p;
    while (len == 0) {
        p = sc->patterns[index];
        if (actions[index].tag != SKIP) len = match(sc->input, p);
        if (len == 0) {
            if (p[1] == '\0') crash("Bug: no pattern match found");
            index++;
        }
    }
    if ((actions[index].tag & 0x80) == 0) for (int i = 0; i < len; i++) {
        *sc->tags++ = MORE;
        sc->input++;
    }
    return index;
}

// Given a state and the current position in the input, find the pattern that
// matches (which should never fail) and mark the matched characters with MORE.
// Update the state and input position. If white space is next in the input, and
// the action is a lookahead, skip forwards to the next token and check for any
// explicit lookahead rule which applies.
static void step(scanner *sc, bool tracing) {
    bool gap = sc->input[0] == ' ' || sc->input[0] == '\n';
    int index = search(sc);
    action *actions = sc->actions[sc->state];
    int tag = actions[index].tag;
    int target = actions[index].target;
    if (gap) {
        int index2 = lookahead(sc);
        if (index2 >= 0) {
            index = index2;
            tag = actions[index].tag;
            target = actions[index].target;
        }
    }
    if (tracing) trace(sc, sc->state, sc->patterns[index], target, tag);
    tag = tag & 0x7F;
    if (tag != MORE) {
        sc->token[0] = tag;
        sc->token = sc->tags;
    }
    sc->state = target;
}

// ---------- Testing ----------------------------------------------------------

// Scan a line, tagging each byte in the tags array, stopping after \n.
void scan(scanner *sc, char *line, char *tags, bool tracing) {
    sc->state = 0;
    sc->input = line;
    sc->tags = tags;
    sc->token = tags;
    while (sc->input[0] != '\0') step(sc, tracing);
}

// Split the text into a list of lines, replacing each newline by a null.
// Skip blank lines or lines starting with two symbols.
char **splitLines(char *text) {
    int nlines = 0;
    for (int i = 0; text[i] != '\0'; i++) if (text[i] == '\n') nlines++;
    char **lines = malloc((nlines + 2) * sizeof(char *));
    int p = 0, row = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i]= '\0';
        if (text[p] != '\0' && (isalnum(text[p]) || isalnum(text[p+1]))) {
            lines[row++] = &text[p];
        }
        p = i + 1;
    }
    lines[row++] = NULL;
    lines[row] = NULL;
    return lines;
}

// Run a test from the test file.
void runTest(scanner *sc, char *line, char *expected, bool tracing) {
    char actual[100];
    scan(sc, line, actual, tracing);
    int n = strlen(line);
    actual[n] = '\0';
    if (strcmp(actual, expected) == 0) return;
    fprintf(stderr, "Error:\n");
    fprintf(stderr, "%s", line);
    fprintf(stderr, "%s (expected)\n", expected);
    fprintf(stderr, "%s (actual)\n", actual);
    exit(1);
}

// Run the tests from the test file.
int runTests(scanner *sc, char *tests, bool tracing) {
    char line[100];
    char tags[100];
    int passes = 0;
    char **lines = splitLines(tests);
    for (int i = 0; lines[i] != NULL && lines[i+1] != NULL; i = i + 2) {
        strcpy(line, lines[i]);
        strcat(line, "\n");
        strcpy(tags, lines[i+1]);
        runTest(sc, line, tags, tracing);
        passes++;
    }
    free(lines);
    return passes;
}

int main(int n, char const *args[n]) {
    char const *lang;
    bool tracing = n > 2;
    if (n == 2) lang = args[1];
    else if (n == 3 && strcmp(args[1], "-t") == 0) lang = args[2];
    else if (n == 3 && strcmp(args[2], "-t") == 0) lang = args[1];
    else crash("Use: ./interpret [-t] lang\n"
        "to read lang/table.bin and run tests from lang/tests.txt");
    char path[100];
    sprintf(path, "%s/table.bin", lang);
    scanner *sc = malloc(sizeof(scanner));
    readScanner(path, sc);
    sprintf(path, "%s/tests.txt", lang);
    char *tests = readFile(path);
    runTests(sc, tests, tracing);
    free(tests);
    free(sc->data);
    free(sc);
    return 0;
}
