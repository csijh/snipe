// Snipe language compiler. Free and open source. See licence.txt.

// Type  ./match x  to compile a description of matching for language x in
// x/match.txt into a scanner table in x/table.bin.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// BIG is the fixed capacity of a list of pointers, including a terminating
// NULL. Increase as necessary. SMALL is the capacity of a state name or pattern
// string or tag name.
enum { BIG = 1024, SMALL = 128 };

// These are the tag symbols currently accepted in descriptions.
char const symbols[] = "()[]{}<>#/\\^$*'\"@=:?-";
// !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~

// A tag is an upper case name or one of the above symbols, which include MORE
// to indicate continuing the current token and BAD to indicate an error token.
// A tag can also be SKIP to mean ignore a table entry as not relevant to the
// current state (also used later in the scanner to tag bytes which continue a
// character), or GAP to tag a space as a gap between tokens or NL to tag a
// newline as a token.
enum tag { MORE = '-', BAD = '?', SKIP = '~', GAP = '_', NL = '.' };

struct matcher {
    char quads[1024][4];
};
typedef struct matcher matcher;

// ----- Reading in  -----------------------------------------------------------

// Read a text file as a string, adding a final newline if necessary, and a null
// terminator. Use binary mode, so that the file size equals the bytes read in.
char *readFile(char const *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("can't read file", 0, path);
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("can't find file size", 0, path);
    char *text = malloc(size+2);
    int n = fread(text, 1, size, file);
    if (n != size) crash("can't read file", 0, path);
    if (n > 0 && text[n - 1] != '\n') text[n++] = '\n';
    text[n] = '\0';
    fclose(file);
    return text;
}

// Validate a line. Check it is ASCII only. Convert '\t' or '\r' to a space. Ban
// other control characters.
void validateLine(int row, char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        unsigned char ch = line[i];
        if (ch == '\t' || ch == '\r') line[i] = ' ';
        else if (ch >= 128) crash("non-ASCII character", row, "");
        else if (ch < ' ' || ch > '~') crash("control character", row, "");
    }
}

// Split a text into a list of lines, replacing each newline by a null.
void splitLines(char *text, char *lines[]) {
    int p = 0, row = 1;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i]= '\0';
        validateLine(row, &text[p]);
        addString(lines, &text[p]);
        p = i + 1;
        row++;
    }
}

// Split a line into a list of tokens.
void splitTokens(language *lang, int row, char *line, char *tokens[]) {
    int start = 0, end = 0, len = strlen(line);
    while (line[start] == ' ') start++;
    while (start < len) {
        end = start;
        while (line[end] != ' ' && line[end] != '\0') end++;
        line[end] = '\0';
        char *pattern = &line[start];
        addString(tokens, pattern);
        start = end + 1;
        while (start < len && line[start] == ' ') start++;
    }
}

// Convert the list of lines into quads
void readQuads(matcher *ma, char *lines[]) {
    char **tokens = malloc(BIG * sizeof(char *));
    tokens[0] = NULL;
    for (int i = 0; lines[i] != NULL; i++) {
        char *line = lines[i];
        splitTokens(lang, i+1, line, tokens);
        readRule(lang, i+1, tokens);
        tokens[0] = NULL;
    }
    free(tokens);
}

int main(int n, char const *args[n]) {
    matcher *ma = malloc(sizeof(matcher));
    char *text = readFile("c/match.txt");
    char **lines = malloc(BIG * sizeof(char *));
    readQuads(ma, lines);
    /*
    if (n != 2) crash("Use: ./compile language", 0, "");
    runTests();
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    char *text = readFile(path);
    language *lang = buildLanguage(text, false);
    printf("%d states, %d patterns\n",
        countStates(lang->states), countPatterns(lang->patterns));
    sprintf(path, "%s/table.bin", args[1]);
    writeTable(lang, path);
    freeLanguage(lang);
    */
    free(lines);
    free(text);
    return 0;
}

/*
Operators are:
=   // match with popped opener, push pair onto matched
+>  // push next onto openers, then check it against closers after caret
>   // mismatch next (including excess) (and push on matched ???)
<   // less: pop and mismatch opener, repeat
~   // incomplete (same as = but don't morph tokens between, usually ?)
->  // skip past ordinary token or anything that can be eaten

Note + and - appear to be forward-biased: + must be opener+opener or
closer+closer, with '+" being both - is that possible? I doubt it, though we do
need '+\ \=. and \=. .+A and other triples that drop . presumably. Minus is
usually '-X or X-' which are unambiguous. '-" is reversible and means either can
appear inside the other and is the default. Note can have #=. and "~. which
means when going backwards, don't know what override to use (leave alone until
opener found, then backtrack - fortunately it is not far) Need asymmetrical .>.
so that the most recent newline is on the stack. Can also have "=" and "~. Do
they override the inner tokens differently? Preferably not, so want ("=" =) and
("~. =).

Seems a genuine problem with newlines going back. Want to push on, then push
other stuff on, then take off when next newline comes along. Maybe it is the
only ambiguity, and resolved when forward meets backward.

What was the problem with normal /* comments that meant an inner /* should be
mismatched?

*/
