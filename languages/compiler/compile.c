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
