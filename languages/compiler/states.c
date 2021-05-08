// Snipe language compiler. Free and open source. See licence.txt.
#include "states.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// An action contains a tag and a target state, each stored in one byte. The tag
// may have the top bit set to indicate a lookahead.
struct action { char tag, target; };
typedef struct action action;

// A state stores the line number on which its starting flag was established, or
// 0 if not established yet, to allow a sensible error message for a clash.
struct state {
    char *name;
    int index;
    bool starting;
    int row;
    action *actions;
    bool visiting, visited;
};
typedef struct state state;

// List of states, plus patterns and SKIP.
struct states { int c, n; state **a; strings *patterns; char SKIP; };

static state *newState(char *name) {
    state *s = malloc(sizeof(state));
    *s = (state) { .name = name, .row = 0, .actions = NULL };
    return s;
}

static void freeState(state *s) {
    if (s->actions != NULL) free(s->actions);
    free(s);
}

states *newStates() {
    states *ss = malloc(sizeof(states));
    state **a = malloc(4 * sizeof(state *));
    *ss = (states) { .n = 0, .c = 4, .a = a, .patterns = NULL };
    return ss;
}

void freeStates(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        freeState(ss->a[i]);
    }
    free(ss->a);
    free(ss);
}

// Find a state by name, or create a new one.
static state *findState(states *ss, char *name) {
    for (int i = 0; i < ss->n; i++) {
        if (strcmp(name, ss->a[i]->name) == 0) return ss->a[i];
    }
    if (ss->n >= ss->c) {
        ss->c = ss->c * 2;
        ss->a = realloc(ss->a, ss->c * sizeof(state *));
    }
    ss->a[ss->n] = newState(name);
    return ss->a[ss->n++];
}

void setType(states *ss, char *name, bool starting, int row) {
    state *s = findState(ss, name);
    if (s->row > 0 && s->starting && ! starting) {
        crash(
            "%s is a starting state because of line %d\n"
            "but can occur between tokens because of line %d",
            name, s->row, row
        );
    }
    if (s->row > 0 && ! s->starting && starting) {
        crash(
            "%s is a continuing state because of line %d\n"
            "but can occur within a token because of line %d",
            name, s->row, row
        );
    }
    s->starting = starting;
    s->row = row;
}

bool isStarting(states *ss, char *name) {
    state *s = findState(ss, name);
    return s->starting;
}

void sortStates(states *ss) {
    for (int i = 1; i < ss->n; i++) {
        state *s = ss->a[i];
        int j = i - 1;
        if (! s->starting) continue;
        while (j >= 0 && ! ss->a[j]->starting) {
            ss->a[j + 1] = ss->a[j];
            j--;
        }
        ss->a[j + 1] = s;
    }
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        if (s->starting && i>=32) crash("more than 32 starting states", 0, "");
        if (i >= 128) crash("more than 128 states", 0, "");
        s->index = i;
    }
}

void setupActions(states *ss, strings *patterns, char SKIP) {
    ss->patterns = patterns;
    ss->SKIP = SKIP;
}

void fillAction(states *ss, char *name, int p, char tag, char *target) {
    state *s = findState(ss, name);
    int n = countStrings(ss->patterns);
    if (s->actions == NULL) {
        s->actions = malloc(n * sizeof(action));
        for (int p = 0; p < n; p++) s->actions[p].tag = ss->SKIP;
    }
    s->actions[p].tag = tag;
    state *t = findState(ss, target);
    s->actions[p].target = t->index;
}

// Check that all states have rules/actions associated with them.
void checkDefined(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        if (s->actions == NULL) crash("state %s not defined", s->name);
    }
}

// Check that each state covers all input characters. Assume each character is
// represented as a one-character pattern.
void checkComplete(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        for (int p = 0; p < countStrings(ss->patterns); p++) {
            char *ps = getString(ss->patterns, i);
            if (strlen(ps) != 1) continue;
            if (s->actions[p].tag == ss->SKIP) {
                if (ps[0] == '\n') ps = "\\n";
                crash("state %s has no rule for character '%s'", s->name, ps);
            }
        }
    }
}

// Visit a state during a depth first search, looking for a loop with no
// progress when the given character is next in the input. A lookahead action
// for a pattern starting with the character indicates a progress-free jump to
// another state. Any (non-skip) action for a pattern which is that single
// character indicates that the character has been dealt with. Return true for
// success, false for a loop.
static bool visit(states *ss, state *s, char ch) {
    if (s->visited) return true;
    if (s->visiting) return false;
    s->visiting = true;
    for (int i = 0; i < countStrings(ss->patterns); i++) {
        char *p = getString(ss->patterns, i);
        if (p[0] < ch) continue;
        if (p[0] > ch) break;
        int tag = s->actions[i].tag;
        if (tag == ss->SKIP) continue;
        bool lookahead = (tag & 0x80) != 0;
        if (lookahead) {
            bool ok = visit(ss, ss->a[s->actions[i].target], ch);
            if (! ok) return false;
        }
        if (strlen(p) == 1) break;
    }
    s->visited = true;
    return true;
}

// Report a progress-free loop of states when ch is next in the input.
void reportLoop(states *ss, char ch) {
    fprintf(stderr, "Error: possible infinite loop with no progress\n");
    fprintf(stderr, "when character '");
    if (ch == '\n') fprintf(stderr, "\\n");
    else fprintf(stderr, "%c", ch);
    fprintf(stderr, "' is next in the input.\n");
    fprintf(stderr, "The states involved are:");
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        if (! s->visiting) continue;
        fprintf(stderr, " %s", s->name);
    }
    fprintf(stderr, "\n");
    exit(1);
}

// For each character, initialise flags and do a depth first search.
void checkProgress(states *ss, strings *patterns) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if (ch > '\n' && ch < ' ') continue;
        for (int i = 1; i < ss->n; i++) {
            state *s = ss->a[i];
            s->visiting = s->visited = false;
        }
        for (int i = 0; i < ss->n; i++) {
            state *s = ss->a[i];
            bool ok = visit(ss, s, ch);
            if (! ok) reportLoop(ss, ch);
        }
    }
}

void writeTable(states *ss, strings *patterns, char const *path) {
    FILE *fp = fopen(path, "wb");
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        fprintf(fp, "%s%c", s->name, '\0');
    }
    fprintf(fp, "%c", '\0');
    for (int i = 0; i < countStrings(ss->patterns); i++) {
        char *p = getString(patterns, i);
        fprintf(fp, "%s%c", p, '\0');
    }
    fprintf(fp, "%c", '\0');
    int np = countStrings(ss->patterns);
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        fwrite(s->actions, 2, np, fp);
    }
    fclose(fp);
}

/*
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
#ifdef statesTest

int main(int argc, char const *argv[]) {
    states *ss = newStates();
    freeStates(ss);
    return 0;
}

#endif
