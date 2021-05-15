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
// string forming a language description (made of concatenated lines), then
// strings which test some generated table entries, then a NULL. Each test
// checks an entry in the generated table, expressed roughly in the original
// rule format, except that each rule has a single pattern and the tag is a
// single character..

// One basic illustrative rule.
char *eg1[] = {
    "start == != start OP\n",
    // ----------------------
    "start == start O",
    "start != start O", NULL
};

// Rule with no tag, continuing the token. Range pattern.
char *eg2[] = {
    "start 0..9 number\n"
    "number 0..9 start VALUE\n",
    //--------------------------
    "start 0 number -",
    "start 5 number -",
    "start 9 number -",
    "number 0 start V",
    "number 5 start V",
    "number 9 start V", NULL
};

// Symbol as token type, e.g. ? for error token.
char *eg3[] = {
    "start \\ escape\n"
    "escape n start ?\n",
    //-------------------
    "start \\ escape -",
    "escape n start ?", NULL
};

// Multiple rules, whichever matches the next input is used.
char *eg4[] = {
    "start == != start OP\n"
    "start a..z A..Z id\n"
    "id a..z A..Z start ID\n",
    //------------------------
    "start == start O",
    "start x id -",
    "id x start I", NULL
};

// Longer pattern takes precedence (= is a prefix of ==). (Can't test here
// whether the patterns are ordered so that == is before =)
char *eg5[] = {
    "start = start SIGN\n"
    "start == != start OP\n",
    //-----------------------
    "start = start S",
    "start == start O", NULL
};

// Earlier rule for same pattern takes precedence (> is matched by last rule).
char *eg6[] = {
    "start < filename\n"
    "filename > start =\n"
    "filename !..~ filename\n",
    //-------------------------
    "start < filename -",
    "filename > start =",
    "filename ! filename -", NULL
};

// Check that, in the named example, the given test succeeds. The test
// represents an action, i.e. an entry in the generated table. It is expressed
// in the original rule format (base state name, single pattern, target state
// name, lookahead marker, single character tag).
void checkAction(states *ss, char *name, char *test) {
    char text[100];
    strcpy(text, test);
    strings *tokens = newStrings();
    splitTokens(1, text, tokens);
    char *base = getString(tokens, 0);
    char *pattern = getString(tokens, 1);
    char *target = getString(tokens, 2);
    char tag = getString(tokens, 3)[0];
    if (tag == '~') tag = (0x80 | getString(tokens, 3)[1]);
    action act = getAction(ss, base, pattern);
    int t = getIndex(ss, target);
    freeStrings(tokens);
    if (act.tag == tag && act.target == t) return;
    fprintf(stderr, "Test failed: %s: %s\n", name, test);
    if ((act.tag & 0x80) != (tag & 0x80)) {
        fprintf(stderr, "lookahead %d\n", (act.tag >> 7));
    }
    if ((act.tag & 0x7F) != (tag & 0x7F)) {
        fprintf(stderr, "tag %c\n", act.tag & 0x7F);
    }
    if (act.target != t) {
        fprintf(stderr, "target %d\n", t);
    }
    exit(1);
}

// Run the tests in an example.
void runExample(char *name, char *eg[], bool print) {
    char text[1000];
    strcpy(text, eg[0]);
    rules *rs = readRules(text);
    states *ss = newStates(rs);
    checkTypes(ss);
    sortStates(ss);
    fillActions(ss);
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
//    runExample("eg7", eg7, false);
//    runExample("eg8", eg8, false);
//    runExample("eg9", eg9, false);
//    runExample("eg10", eg10, false);
//    runExample("eg11", eg11, false);
//    runExample("eg12", eg12, false);
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


// A lookahead rule allows a token's type to be affected by the next token.
char *eg7[] = {
    "start a..z A..Z id\n"
    "id a..z A..Z 0..9 id\n"
    "id ( start ~FUN\n"
    "id start ID\n",
    //--------------
    "start f id -",
    "id ( start ~FUN",
    "id ; start ~ID", NULL
};

// A lookahead rule can be a continuing one. It is a jump to another state.
char *eg8[] = {
    "start a start ID\n"
    "start . start2 ~-\n"
    "start2 . start2 DOT\n"
    "start2 start\n",
    //-------------------
    "start . start2 ~-", NULL
};

// Identifier may start with keyword. A default rule ("matching the empty
// string") is turned into a lookahead rule for each unhandled character.
char *eg9[] = {
    "start a..z A..Z id\n"
    "start if else for while key\n"
    "key a..z A..Z 0..9 id\n"
    "key start KEY\n"
    "id a..z A..Z 0..9 id\n"
    "id start ID\n",
    //--------------
    "start f id -",
    "start for key -",
    "key m id -",
    "key ; start ~KEY", NULL
};

// A default rule with no tag is an unconditional jump.
char *eg10[] = {
    "start #include inclusion KEY\n"
    "inclusion < filename\n"
    "inclusion start\n"
    "filename > start QUOTED\n"
    "filename !..~ filename\n"
    "filename start ?\n",
    //--------------
    "start #include inclusion K",
    "inclusion < filename -",
    "inclusion ! start ~-",
    "inclusion x start ~-",
    "inclusion ~ start ~-",
    "inclusion \\n inclusion .",
    "inclusion \\s inclusion _", NULL
};

// Can have multiple starting states. Check that the default is to accept any
// single character as an error token and stay in the same state.
char *eg11[] = {
    "start # hash KEY\n"
    "hash include start RESERVED\n"
    "html <% java <\n"
    "java %> html >\n",
    //--------------
    "start # hash K",
    "hash include start R",
    "hash x hash ?",
    "hash i hash ?",
    "hash \\n hash .",
    "hash \\s hash _",
    "html <% java <",
    "html x html ?",
    "java %> html >",
    "java x java ?", NULL
};

// Corrected.
char *eg12[] = {
    "start . dot\n"
    "dot 0..9 start NUM\n"
    "dot a..z A..Z prop ~SIGN\n"
    "prop a..z A..Z prop2\n"
    "prop start\n"
    "prop2 a..z A..Z 0..9 prop2\n"
    "prop2 start PROPERTY\n",
    //-----------------------
    "dot 0 start N",
    "dot x prop ~S",
    "prop x prop2 -",
    "prop2 x prop2 -",
    "prop2 ; start ~P", NULL
};

// Illustrates lookahead. If a state has an explicit lookahead, a rule "_ s \s
// s" is added to tell the scanner to look past the spaces and match what comes
// next with any lookahead patterns in the state.
char *eg13[] = {
    "start a..z id\n"
    "id a..z id\n"
    "id ( start ~FUN\n"
    "id start ID\n",
    //--------------
    "id \\s start ~I",
    "id \\n start ~I", NULL
};

// CAUSES ERROR. The number state is not defined.
char *eg14[] = {
    "start . dot\n"
    "dot 0..9 number\n"
    "SIGN dot a..z A..Z prop\n"
    "prop a..z A..Z 0..9 prop\n"
    "prop start PROPERTY\n",
    //----------------------
    "dot 0 number -", NULL
};

// CAUSES ERROR. The prop state is a starting state because
// of line 3, and continuing state because of lines 4/5.
char *eg15[] = {
    "start . dot\n"
    "dot 0..9 start NUM\n"
    "SIGN dot a..z A..Z prop\n"
    "prop a..z A..Z 0..9 prop\n"
    "prop start PROPERTY\n",
    //----------------------
    "dot 0 start N", NULL
};

// CAUSES ERROR. An explicit lookahead rule in a continuing state must end the
// current token.
char *eg16[] = {
    "start a id\n"
    "- id ( id2\n"
    "id2 a id2\n"
    "id2 start ID\n",
    //----------------------
    "- id ( id2", NULL
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
