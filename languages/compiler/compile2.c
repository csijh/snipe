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
#include "rules.h"
#include "states.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// Create a state object for each state name. Classify each state.
states *createStates(rules *rs) {
    states *ss = newStates();
    strings *names = newStrings();
    stateNames(rs, names);
    for (int i = 0; i < countStrings(names); i++) {
        char *state = getString(names, i);
        addState(ss, state);
        setType(ss, state, isStarting(rs, state));
    }
    freeStrings(names);
    return ss;
}

// Convert rules into actions on states.
void convertRules(rules *rs, states *ss) {
    sortStates(ss);
    strings *ps = getPatterns(rs);
    setupActions(ss, ps);
    for (int i = 0; i < countRules(rs); i++) {
        rule *r = getRule(rs, i);
        char tag = r->tag[0];
        if (r->lookahead) tag = (0x80 | tag);
        convert(ss, r->row, r->base, r->patterns, r->target, tag);
    }
}

int main(int n, char const *args[n]) {
    if (n != 2) crash("Use: ./compile language", 0, "");
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    rules *rs = readRules(path);
    states *ss = createStates(rs);
    convertRules(rs, ss);
    checkComplete(ss);
    checkProgress(ss);
    sprintf(path, "%s/table.bin", args[1]);
    writeTable(ss, path);
    freeStates(ss);
    freeRules(rs);
}

/*
// ----- Defaults --------------------------------------------------------------

// Add implicit defaults for a starting state. If there is an explicit default
// rule (an unconditional jump to another starting state) or a rule which
// mentions the range !..~ then all characters are covered. If not, add a rule
// which accepts any single character as an error. Add space and newline rules.
void addStartingDefaults(language *lang, state *s) {
    bool ok = false;
    for (int j = 0; s->rules[j] != NULL; j++) {
        rule *r = s->rules[j];
        if (r->type == DEFAULT) ok = true;
        if (r->patterns[0] == NULL) continue;
        if (strcmp(r->patterns[0]->name, "!..~") == 0) ok = true;
    }
    if (! ok) {
        char *bad[] = { s->name, "!..~", s->name, lang->bad->name, NULL};
        readRule(lang, 0, bad);
    }
    char *sp[] = { s->name, " ", s->name, lang->gap->name, NULL};
    readRule(lang, 0, sp);
    char *nl[] = { s->name, "\n", s->name, lang->newline->name, NULL};
    readRule(lang, 0, nl);
}

// Add implicit defaults for a continuing rule. If necessary add "s start ?".
// Add "D s \s d" and "D s \n d" where D and d are from the default rule. Note
// that in the scanner, if a space or newline is next, a search is made for any
// other lookahead rule that might apply.
void addContinuingDefaults(language *lang, state *s) {
    int n = countRules(s->rules);
    rule *d = s->rules[n-1];
    if (d->type != DEFAULT) {
        state *s0 = lang->states[0];
        char *df[] = { s->name, s0->name, lang->bad->name, NULL };
        readRule(lang, 0, df);
        d = s->rules[n];
    }
    char tag[SMALL+1];
    sprintf(tag, "~%s", d->tag->name);
    char *df[] = { s->name, " ", d->target->name, tag, NULL };
    readRule(lang, 0, df);
    char *nl[] = { s->name, "\n", d->target->name, tag, NULL };
    readRule(lang, 0, nl);
}

// Combine the above functions to add the default rules to the language.
void addDefaults(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (s->added) continue;
        if (s->starting) addStartingDefaults(lang, s);
        else addContinuingDefaults(lang, s);
    }
}

// ----- Interim printing ------------------------------------------------------

// Print out a state in original style, to conform additions.
void printState(language *lang, state *s) {
    for (int i = 0; s->rules[i] != 0; i++) {
        rule *r = s->rules[i];
        if (r->row == 0) printf("%-4s", "+");
        else printf("%-4d", r->row);
        if (r->type == LOOKAHEAD) printf("%s ", r->tag->name);
        printf("%s", s->name);
        for (int j = 0; r->patterns[j] != NULL; j++) {
            pattern *p = r->patterns[j];
            if (p->name[0] == '\n') printf(" \\n");
            else if (p->name[0] == ' ') printf(" \\s");
            else if (p->name[0] == '\\') printf(" \\%s", p->name);
            else printf(" %s", p->name);
        }
        printf(" %s", r->target->name);
        if (r->type != LOOKAHEAD) printf(" %s", r->tag->name);
        printf("\n");
    }
}

// Print out all the states, to confirm additions before generating actions.
void printLanguage(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        printState(lang, lang->states[i]);
        printf("\n");
    }
}

// ----- Sorting --------------------------------------------------------------

// Sort the states so that the starting states come before the continuing
// states. Limit the number of starting states to 32, and the total number to
// 128. Add indexes.
void sortStates(state *states[]) {
    for (int i = 1; states[i] != NULL; i++) {
        state *s = states[i];
        int j = i - 1;
        if (! s->starting) continue;
        while (j >= 0 && ! states[j]->starting) {
            states[j + 1] = states[j];
            j--;
        }
        states[j + 1] = s;
    }
    for (int i = 0; states[i] != NULL; i++) {
        state *s = states[i];
        if (s->starting && i>=32) crash("more than 32 starting states", 0, "");
        if (i >= 128) crash("more than 128 states", 0, "");
        s->index = i;
    }
}

// Check whether a pattern string is a range of characters.
bool isRange(char *s) {
    return strlen(s) == 4 && s[1] == '.' && s[2] == '.';
}

// Compare two patterns in ASCII order, except prefer longer strings and
// put ranges last.
int compare(pattern *p, pattern *q) {
    char *s = p->name, *t = q->name;
    if (isRange(s) && ! isRange(t)) return 1;
    if (isRange(t) && ! isRange(s)) return -1;
    for (int i = 0; ; i++) {
        if (s[i] == '\0' && t[i] == '\0') break;
        if (s[i] == '\0') return 1;
        if (t[i] == '\0') return -1;
        if (s[i] < t[i]) return -1;
        if (s[i] > t[i]) return 1;
    }
    return 0;
}

// Sort the patterns. Add indexes.
void sortPatterns(pattern *patterns[]) {
    for (int i = 1; patterns[i] != NULL; i++) {
        pattern *p = patterns[i];
        int j = i - 1;
        while (j >= 0 && compare(patterns[j], p) > 0) {
            patterns[j + 1] = patterns[j];
            j--;
        }
        patterns[j + 1] = p;
    }
    for (int i = 0; patterns[i] != NULL; i++) patterns[i]->index = i;
}

// Remove ranges from the patterns array, and add them to the tags array so they
// get freed at the end.
void removeRanges(language *lang) {
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (! isRange(p->name)) continue;
        lang->patterns[i] = NULL;
        addPattern(lang->tags, p);
    }
}

// Sort the states and patterns.
void sort(language *lang) {
    sortStates(lang->states);
    sortPatterns(lang->patterns);
    removeRanges(lang);
}

// ----- Actions ---------------------------------------------------------------

// Initialize all actions to SKIP.
void fillSkipActions(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        for (int j = 0; lang->patterns[j] != NULL; j++) {
            s->actions[j].tag = SKIP;
            s->actions[j].target = 0;
        }
    }
}

// Fill in an action for a particular rule and pattern, if not already filled
// in. For a lookahead or default rule, set the top bit of the tag byte.
void fillAction(language *lang, rule *r, pattern *p) {
    state *s = r->base;
    int n = p->index;
    if (s->actions[n].tag != SKIP) return;
    s->actions[n].tag = r->tag->name[0];
    if (r->type != MATCHING) s->actions[n].tag |= 0x80;
    s->actions[n].target = r->target->index;
}

// Fill in a range of one-character actions for a rule.
void fillRangeActions(language *lang, rule *r, char from, char to) {
    for (char ch = from; ch <= to; ch++) {
        if (ch < '\n' || ('\n' < ch && ch < ' ') || ch > '~') continue;
        pattern *p = findPattern1(lang, ch);
        fillAction(lang, r, p);
    }
}

// Fill in the actions for a rule, for patterns not already handled.
void fillRuleActions(language *lang, rule *r) {
    if (r->type == DEFAULT) fillRangeActions(lang, r, '!', '~');
    else for (int i = 0; r->patterns[i] != NULL; i++) {
        pattern *p = r->patterns[i];
        if (isRange(p->name)) fillRangeActions(lang, r, p->name[0], p->name[3]);
        else fillAction(lang, r, p);
    }
}

// Fill in all actions.
void fillActions(language *lang) {
    fillSkipActions(lang);
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        fillRuleActions(lang, r);
    }
}

// ---------- Progress ---------------------------------------------------------

// Before doing a search to check progress for a character, set the visit flags
// to false.
void startSearch(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        s->visiting = s->visited = false;
    }
}

// Visit a state during a depth first search, looking for a loop with no
// progress when the given character is next in the input. A lookahead action
// for a pattern starting with the character indicates a progress-free jump to
// another state. Any (non-skip) action for a pattern which is that single
// character indicates that the character has been dealt with. Return true for
// success, false for a loop.
bool visit(language *lang, state *s, char ch) {
    if (s->visited) return true;
    if (s->visiting) return false;
    s->visiting = true;
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (p->name[0] < ch) continue;
        if (p->name[0] > ch) break;
        int tag = s->actions[i].tag;
        if (tag == SKIP) continue;
        bool lookahead = (tag & 0x80) != 0;
        if (lookahead) {
            bool ok = visit(lang, lang->states[s->actions[i].target], ch);
            if (! ok) return false;
        }
        if (strlen(p->name) == 1) break;
    }
    s->visited = true;
    return true;
}

// Report a progress-free loop of states when ch is next in the input.
void reportLoop(language *lang, char ch) {
    fprintf(stderr, "Error: possible infinite loop with no progress\n");
    fprintf(stderr, "when character '");
    if (ch == '\n') fprintf(stderr, "\\n");
    else fprintf(stderr, "%c", ch);
    fprintf(stderr, "' is next in the input.\n");
    fprintf(stderr, "The states involved are:");
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (! s->visiting) continue;
        fprintf(stderr, " %s", s->name);
    }
    fprintf(stderr, "\n");
    exit(1);
}

// For each character, carry out a depth first search for a loop of
// progress-free jumps between states when that character is next in the input.
// Report any loop found.
void checkProgress(language *lang) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if (ch > '\n' && ch < ' ') continue;
        startSearch(lang);
        for (int i = 0; lang->states[i] != NULL; i++) {
            state *s = lang->states[i];
            bool ok = visit(lang, s, ch);
            if (! ok) reportLoop(lang, ch);
        }
    }
}

// Read a language, analyse, add defaults, sort, fill actions, check progress.
// Optionally print before sorting.
language *buildLanguage(char *text, bool print) {
    language *lang = readLanguage(text);
    checkLanguage(lang);
    addDefaults(lang);
    if (print) printLanguage(lang);
    sort(lang);
    fillActions(lang);
    checkProgress(lang);
    return lang;
}

// ---------- Output -----------------------------------------------------------

// Write out a binary file containing the names of the states as null-terminated
// strings, then a null, then the pattern strings, then a null, then the array
// of actions for each state.
void writeTable(language *lang, char const *path) {
    FILE *fp = fopen(path, "wb");
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        fprintf(fp, "%s%c", s->name, '\0');
    }
    fprintf(fp, "%c", '\0');
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        fprintf(fp, "%s%c", p->name, '\0');
    }
    fprintf(fp, "%c", '\0');
    int np = countPatterns(lang->patterns);
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        fwrite(s->actions, 2, np, fp);
    }
    fclose(fp);
}

// ----- Testing ---------------------------------------------------------------

// Check that, in the named example, the given test succeeds. The test
// represents an action, i.e. an entry in the table to be generated. It is
// expressed roughly in the original rule format (base state name, single
// pattern or \n or \s, target state name, single character tag at either end).
void checkAction(language *lang, char *name, char *test) {
    char text[100];
    strcpy(text, test);
    char *tokens[5] = { NULL };
    splitTokens(lang, 0, text, tokens);
    state *s, *t;
    pattern *p;
    unsigned char tag;
    int ibase = 0, ipat = 1, itarget = 2, itag = 3;
    s = findState(lang, tokens[ibase]);
    if (strcmp(tokens[ipat],"\\n") == 0) tokens[ipat] = "\n";
    if (strcmp(tokens[ipat],"\\s") == 0) tokens[ipat] = " ";
    p = findPattern(lang, tokens[ipat]);
    t = findState(lang, tokens[itarget]);
    if (tokens[itag][0] == '~') tag = tokens[itag][1] | 0x80;
    else tag = tokens[itag][0];
    unsigned char actionTag = s->actions[p->index].tag;
    int actionTarget = s->actions[p->index].target;
    if (actionTag == tag && actionTarget == t->index) return;
    fprintf(stderr, "Test failed: %s: %s\n", name, test);
    if ((actionTag & 0x80) != (tag & 0x80)) {
        fprintf(stderr, "lookahead %d\n", (actionTag >> 7));
    }
    if ((actionTag & 0x7F) != (tag & 0x7F)) {
        fprintf(stderr, "tag %c\n", actionTag & 0x7F);
    }
    if (actionTarget != t->index) {
        fprintf(stderr, "target %s\n", lang->states[actionTarget]->name);
    }
    exit(1);
}

// Run the tests in an example.
void runExample(char *name, char *eg[], bool print) {
    char text[1000];
    strcpy(text, eg[0]);
    language *lang = buildLanguage(text, print);
    for (int i = 1; eg[i] != NULL; i++) checkAction(lang, name, eg[i]);
    freeLanguage(lang);
}

// Examples from help/languages.xhtml. Each example has a string forming a
// language description (made of concatenated lines), then strings which test
// some generated table entries, then a NULL. Each test checks an entry in the
// generated table, expressed roughly in the original rule format (state name,
// single pattern or \n or \s, state name, single character tag at either end).

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

// Symbol as token type, i.e. ? for error token.
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
    runExample("eg13", eg13, false);

//    runExample("eg14", eg14, false);
//    runExample("eg15", eg15, false);
//    runExample("eg16", eg16, false);
//    runExample("eg17", eg17, false);
}

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
