// Snipe language compiler. Free and open source. See licence.txt.

// Type  ./match x  to compile a description of matching for language x in
// x/match.txt into a scanner table in x/table.bin.

#include "list.h"
#include "data.h"
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

void readRule(matcher *ma, char *tokens[]) {
    int n = countStrings(tokens);
    if (n == 0) return;
    char tag = '-';
    if (strlen(tokens[n-1]) == 1) {
        tag = tokens[n-1][0];
        n--;
    }
    for (int i = 0; i < n; i++) {
        char *triple = tokens[i];
        if (strlen(triple) != 3) crash("expecting triple of symbols");
    }
}

// Convert the list of lines into quads
void readRules(char *lines[]) {
    char **tokens = newStrings();
    for (int i = 0; i < countStrings(lines); i++) {
        char *line = lines[i];
        printf("line %d %s\n", i+1, line);
        tokens = splitTokens(i+1, line, tokens);
//        readRule(i+1, tokens);
        clearStrings(tokens);
    }
    freeStrings(tokens);
}

int main(int n, char const *args[n]) {
    matcher *ma = malloc(sizeof(matcher));
    char *text = readFile("../c/match.txt", false);
    char **lines = splitLines(text);
    readRules(ma, lines);
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
    free(ma);
    return 0;
}

/*
Operators are:
=   // match with popped opener, push pair onto matched
+   // push next onto openers, then check it against closers after caret
>   // mismatch next (including excess) (and push on matched ???)
<   // less: pop and mismatch opener, repeat
~   // special form of = indicating careful backwards algorithm

First, use = triples to classify tags as opener, closer or both and as bracket
(no override) or delimiter. Check nothing is both a bracket and delimiter. Check
that a delimiter always causes the same overriding (excluding RHS of ~). Check
that + never overrides.

Add defaults -+( ->) for every opener and non-opener, )+- (<- for every closer
and non-closer. Deduce precedence (<[ ]>) for every pair of open brackets, and
pair of close brackets. Add (+[ ]+) for every pair of open-bracket and opener,
pair of closer and close-bracket. Add <>X X<> * for open/close delimiters, for
all tags X other than <=> and <+<. Add .$X for everything except close
delimiters to mean search backwards from . to preceding . accepting last # or
similar if any.

Note + and - appear to be forward-biased: + must be opener+opener or
closer+closer, with '+" being both - is that possible? I doubt it. Note can have
#=. and "~. which means when going backwards, don't know what override to use
(leave alone until opener found, then backtrack - fortunately it is not far) Can
also have "=" and "~. Do they override the inner tokens differently? No.

*/
