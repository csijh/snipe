// Snipe language compiler. Free and open source. See licence.txt.

// Type
//       ./compile x
//       ./interpret x
//
// to compile a description of language x in x/rules.txt into a scanner table in
// x/table.bin, and then run tests on the table.

// A rule has a base state, patterns, a target state, and an optional tag. A
// pattern may be a range such as a..z to represent single-character patterns. A
// backslash followed by digits can be used to specify a control character or
// space. A tilde ~ prefix on the tag indicates a lookahead rule, and a lack of
// patterns indicates a default rule.

// Each state must consistently be either a starting state between tokens, or a
// continuing state within tokens. There is a search to make sure there are no
// cycles which fail to make progress. A default rule is treated as a lookahead
// for any single-character patterns not already covered. A check is made that
// each state is complete, covering every possible input character. That means
// the state machine operation is uniformly driven by the next input character.

// The resulting table has an entry for each state and pattern, with a tag and a
// target. The tag is a token type to label and terminate the current token, or
// indicates continuing the token, skipping the table entry, or classifying a
// text byte as a continuing character of a token or a space or a newline. The
// tag can have its top bit set to indicate lookahead behaviour rather than
// normal matching behaviour. If a state has any explicit lookahead rules, then
// matching a space is marked as a lookahead.

// The states are sorted with starting states first, and the number of starting
// states is limited to 32 so they can be cached by the scanner (in the tag
// bytes for spaces). The total number of states is limited to 128 so a state
// index can be held in a char. The patterns are sorted, with longer ones before
// shorter ones, so the next character in the input can be used to find the
// first pattern starting with that character. The patterns are searched
// linearly, skipping the ones where the table entry has SKIP, to find the first
// match. Completeness ensures that the search always succeeds.

#include "strings.h"
#include "states.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// For testing, examples are taken from help/languages.xhtml. Each example has a
// string (made of concatenated lines) forming a language description, then
// strings which test some generated table entries, then a NULL. Each test
// checks an entry in the generated table, expressed roughly in the original
// rule format, except that each rule has a single pattern and the tag is a
// single character and an explicit . is used to mean continue.

// Y A basic example.
char *eg1[] = {
    "start == != start OP\n"
    "start start ERROR\n",
    // ----------------------
    "start == start OP",
    "start != start OP", NULL
};

// Y Rule with no tag, continuing the token.
char *eg2[] = {
    "start 0..9 number\n"
    "start start ERROR\n"
    "number 0..9 start VALUE\n"
    "number start ERROR\n",
    //--------------------------
    "start 0 number",
    "start 5 number",
    "start 9 number",
    "number 0 start VALUE",
    "number 5 start VALUE",
    "number 9 start VALUE", NULL
};

// N Multiple rules, whichever matches the next input is used.
char *eg3[] = {
    "start == != start OP\n"
    "start a..z A..Z id\n"
    "start start ERROR\n"
    "id a..z A..Z start ID\n"
    "id start ID\n",
    //------------------------
    "start == start OP",
    "start x id",
    "id x start ID", NULL
};

// Y Longer pattern takes precedence (= is a prefix of ==). (Can't test here
// whether the patterns are ordered so that == is before =)
char *eg4[] = {
    "start = start SIGN\n"
    "start == != start OP\n"
    "start start ERROR\n",
    //-----------------------
    "start = start SIGN",
    "start == start OP", NULL
};

// Y Earlier rule for same pattern takes precedence (> is matched by last rule).
char *eg5[] = {
    "start < filename\n"
    "start start ERROR\n"
    "filename > start QUOTE\n"
    "filename filename\n",
    //-------------------------
    "start < filename",
    "filename > start QUOTE",
    "filename ! filename", NULL
};

// Y A lookahead rule allows a token's type to be affected by what follows.
char *eg6[] = {
    "start a..z A..Z id\n"
    "start start ERROR\n"
    "id a..z A..Z 0..9 id\n"
    "id ( start FUN+\n"
    "id start ID+\n",
    //--------------
    "start f id",
    "id ( start FUN+",
    "id ; start ID+", NULL
};

// Y A lookahead rule can be a continuing one. It is a jump to another state.
char *eg7[] = {
    "start a start ID\n"
    "start . start2 +\n"
    "start start ERROR\n"
    "start2 . start2 DOT\n"
    "start2 start +\n",
    //-------------------
    "start . start2 +", NULL
};

// Y Identifier may start with keyword.
char *eg8[] = {
    "start a..z A..Z id\n"
    "start if else for while key\n"
    "start start ERROR\n"
    "key a..z A..Z 0..9 id\n"
    "key start KEY+\n"
    "id a..z A..Z 0..9 id\n"
    "id start ID+\n",
    //--------------
    "start f id",
    "start for key",
    "key m id",
    "key ; start KEY+", NULL
};

// N A default rule with no type is an unconditional jump.
char *eg9[] = {
    "start #include inclusion KEY\n"
    "start start ERROR\n"
    "inclusion < filename\n"
    "inclusion start +\n"
    "filename > start QUOTE\n"
    "filename !..~ filename\n"
    "filename start ERROR+\n",
    //--------------
    "start #include inclusion KEY",
    "inclusion < filename",
    "inclusion ! start +",
    "inclusion x start +",
    "inclusion ~ start +", NULL
};

// Y Can have multiple starting states.
char *eg10[] = {
    "start # hash KEY\n"
    "start start ERROR\n"
    "hash include start RESERVED\n"
    "hash start +\n"
    "html <% java BRACKET6\n"
    "html html ERROR\n"
    "java %> html BRACKET7\n"
    "java java ERROR\n",
    //--------------
    "start # hash KEY",
    "hash include start RESERVED",
    "hash x start +",
    "hash i start +",
    "html <% java BRACKET6",
    "html x html ERROR",
    "java %> html BRACKET7",
    "java x java ERROR", NULL
};

// N Corrected.
char *eg11[] = {
    "start . dot\n"
    "start start ERROR\n"
    "dot 0..9 start NUM\n"
    "dot a..z A..Z prop SIGN+\n"
    "dot start ERROR+\n"
    "prop a..z A..Z prop2\n"
    "prop start +\n"
    "prop2 a..z A..Z 0..9 prop2\n"
    "prop2 start PROPERTY+\n",
    //-----------------------
    "dot 0 start NUM",
    "dot x prop SIGN+",
    "prop x prop2",
    "prop2 x prop2",
    "prop2 ; start PROPERTY+", NULL
};

// N Illustrates lookahead.
char *eg12[] = {
    "start a..z id\n"
    "start start ERROR\n"
    "id a..z id\n"
    "id ( start FUN+\n"
    "id start ID+\n",
    //--------------
    "id \10 start ID+",
    "id \32 start ID+", NULL
};

// Check that, in the named example, the given test succeeds. The test
// represents an action, i.e. an entry in the generated table. It is expressed
// in the original rule format (base state name, single pattern, target state
// name, token type, lookahead marker).
void checkAction(states *ss, char *name, char *test) {
    char text[100];
    strcpy(text, test);
    strings *tokens = newStrings();
    splitTokens(1, text, tokens);
    char *base = getString(tokens, 0);
    char *pattern = getString(tokens, 1);
    char *target = getString(tokens, 2);
    char *type = "";
    bool lookahead = false;
    if (countStrings(tokens) >= 4) {
        type = getString(tokens, 3);
        int n = strlen(type);
        if (type[n-1] == '+') {
            lookahead = true;
            type[n-1] = '\0';
        }
    }
    freeStrings(tokens);
    action act = getAction(ss, base, pattern);
    int t = getIndex(ss, target);
    bool l = act.op & 0x80;
    char *s = getType(ss, act.op & 0x7F);
    bool okType = (strcmp(type, s) == 0);
    bool okLook = (lookahead == l);
    bool okTarget = (act.target == t);
    if (okType && okLook && okTarget) return;
    fprintf(stderr, "Test failed: %s: %s\n", name, test);
    if (! okLook) {
        fprintf(stderr, "lookahead %d not %d\n", l, lookahead);
    }
    if (! okType) {
        fprintf(stderr, "type %s not %s\n", s, type);
    }
    if (! okTarget) {
        fprintf(stderr, "target %d not %d\n", act.target, t);
    }
    exit(1);
}

// Run the tests in an example.
void runExample(char *name, char *eg[], bool print) {
    char text[1000];
    strcpy(text, eg[0]);
    rules *rs = readRules(text);
    states *ss = newStates(rs);
    char *e = checkAndFillActions(ss);
    if (e != NULL) { printf("(%s) %s\n", name, e); exit(1); }
    for (int i = 1; eg[i] != NULL; i++) checkAction(ss, name, eg[i]);
    freeStates(ss);
    freeRules(rs);
}

// Run all the tests. Keep the last few commented out during normal operation
// because they test error messages.
void runTests() {
    runExample("eg1", eg1, false);
    runExample("eg2", eg2, false);
    runExample("eg3", eg3, false);
    runExample("eg4", eg4, false);
    runExample("eg5", eg5, false);
    runExample("eg6", eg6, false);
    runExample("eg7", eg7, false);
    runExample("eg8", eg8, false);
    runExample("eg9", eg9, false);
    runExample("eg10", eg10, false);
    runExample("eg11", eg11, false);
    runExample("eg12", eg12, false);
//    runExample("eg13", eg13, false);

//    runExample("eg14", eg14, false);
//    runExample("eg15", eg15, false);
//    runExample("eg16", eg16, false);
//    runExample("eg17", eg17, false);
}

int main(int n, char const *args[n]) {
    runTests();
    /*
    if (n != 2) crash("Use: ./compile language", 0, "");
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    char *text = readFile(path, false);
    rules *rs = readRules(text);
    states *ss = newStates(rs);
    checkTypes(ss);
    sortStates(ss);
    fillActions(ss);
    checkComplete(ss);
    checkProgress(ss);
//    sprintf(path, "%s/table.bin", args[1]);
//    writeTable(ss, path);
    freeStates(ss);
    freeRules(rs);
    free(text);
    */
}

/*

// ----- Testing ---------------------------------------------------------------


// CAUSES ERROR. An explicit lookahead rule in a continuing state must end the
// current token.
char *eg16[] = {
    "start a id\n"
    "id ( id2 +\n"
    "id2 a id2\n"
    "id2 start ID\n",
    //----------------------
    "id ( id2 +", NULL
};

// CAUSES ERROR. An explicit lookahead rule in a starting state must not end the
// current token.
char *eg17[] = {
    "DOT start . start2\n"
    "start2 start\n",
    //----------------------
    "DOT start . start2", NULL
};


int main(int n, char const *args[n]) {
    if (n != 2) crash("Use: ./compile language", 0, "");
    runTests();
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    char *text = readFile(path);
    printf("before bL\n");
    language *lang = buildLanguage(text, false);
    printf("after bL\n");
    printf("%d states, %d patterns\n",
        countStates(lang->states), countPatterns(lang->patterns));
    sprintf(path, "%s/table.bin", args[1]);
    writeTable(lang, path);
    freeLanguage(lang);
    free(text);
    return 0;
}
*/
