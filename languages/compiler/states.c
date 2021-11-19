// Snipe language compiler. Free and open source. See licence.txt.
#include "states.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

// A state has a name, an index, a flag to say if it is a starting or continuing
// state, an array of actions, and a couple of depth-first-search flags.
struct state {
    char *name;
    int index;
    bool starting;
    action *actions;
    bool visiting, visited;
};
typedef struct state state;

enum { SKIP = 0xFF };

// Lists of rules and patterns and types, and array of states.
struct states { rules *rs; strings *patterns, *types; int n; state *a[]; };

// Space for an error message.
static char message[1000];

static char *error(char const *fmt, ...) {
    char *m = message;
    m += sprintf(m, "Error: ");
    va_list args;
    va_start(args, fmt);
    m += vsprintf(m, fmt, args);
    va_end(args);
	return message;
}

static void freeState(state *s) {
    if (s->actions != NULL) free(s->actions);
    free(s);
}

// Find the distinct names of base states.
static void findNames(rules *rs, strings *names) {
    for (int i = 0; i < countRules(rs); i++) {
        rule *r = getRule(rs, i);
        char *name = r->base;
        findOrAddString(names, name);
    }
}

// Find a state by name.
static state *findState(states *ss, char *name) {
    for (int i = 0; i < ss->n; i++) {
        if (strcmp(name, ss->a[i]->name) == 0) return ss->a[i];
    }
    return NULL;
}

states *newStates(rules *rs) {
    strings *names = newStrings();
    findNames(rs, names);
    int n = countStrings(names);
    states *ss = malloc(sizeof(states) + n * sizeof(state *));
    strings *ps = getPatterns(rs), *ts = getTypes(rs);
    *ss = (states) { .n = n, .rs = rs, .patterns = ps, .types = ts };
    int np = countStrings(ss->patterns);
    for (int i = 0; i < countStrings(names); i++) {
        char *name = getString(names, i);
        state *s = malloc(sizeof(state));
        s->name = name;
        s->actions = malloc(np * sizeof(action));
        s->index = i;
        for (int p = 0; p < np; p++) s->actions[p] = (action) { SKIP, 0 };
        ss->a[i] = s;
    }
    freeStrings(names);
    return ss;
}

void freeStates(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        freeState(ss->a[i]);
    }
    free(ss);
}

// Check if a rule terminates the current token.
static bool isTerminating(rule *r) {
    return strlen(r->type) > 0;
}

// For each target state, check whether it is defined.
static char *checkDefined(states *ss) {
    for (int i = 0; i < countRules(ss->rs); i++) {
        rule *r = getRule(ss->rs, i);
        char *name = r->target;
        state *s = findState(ss, name);
        if (s != NULL) continue;
        return error("undefined state '%s' on line %d", name, r->row);
    }
    return NULL;
}

// Check whether rule r shows that state s is a starting state, i.e. s is the
// base of the first rule, or the target of a rule with a token type.
static bool showsStarting(state *s, rule *r) {
    if (strcmp(s->name, r->base) == 0 && r->row == 1) return true;
    if (strcmp(s->name, r->target) == 0 && isTerminating(r)) return true;
    return false;
}

// Check whether rule r shows that state s is a continuing state, i.e. s is the
// base of a terminating lookahead rule, or the target of a non-lookahead
// non-terminating rule.
static bool showsContinuing(state *s, rule *r) {
    if (strcmp(s->name, r->base) == 0) {
        if (r->lookahead && isTerminating(r)) return true;
    }
    if (strcmp(s->name, r->target) == 0) {
        if (! r->lookahead && ! isTerminating(r)) return true;
    }
    return false;
}

// Check whether each state is a starting state or continuing state, and check
// consistency. The first row which shows a state is a starting state (or zero),
// and the same for continuing, are tracked, to produce a helpful message.
static char *classify(state *s, rules *rs) {
    int rowS = 0, rowC = 0;
    for (int i = 0; i < countRules(rs); i++) {
        rule *r = getRule(rs, i);
        if (showsStarting(s, r)) { if (rowS == 0) rowS = r->row; }
        if (showsContinuing(s, r)) { if (rowC == 0) rowC = r->row; }
    }
    if (rowS > 0 && rowC > 0) {
        char *message =
            "%s is a starting state (line %d) "
            "and a continuing state (line %d)";
        return error(message, s->name, rowS, rowC);
    }
    s->starting = (rowC == 0);
    return NULL;
}

// Check the classification of each state.
static char *checkClassification(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        char *m = classify(s, ss->rs);
        if (m != NULL) return m;
    }
    return NULL;
}

// A lookahead rule which continues a token is a jump. It must have base and
// target states which are both starting or both continuing.
static char *checkJumps(states *ss) {
    for (int i = 0; i < countRules(ss->rs); i++) {
        rule *r = getRule(ss->rs, i);
        state *base = findState(ss, r->base);
        state *target = findState(ss, r->target);
        if (! r->lookahead || isTerminating(r)) continue;
        if (base->starting && ! target->starting) {
            char *m = "line %d jumps from a starting to a continuing state";
            return error(m, r->row);
        }
        else if (! base->starting && target->starting) {
            char *m = "line %d jumps from a continuing to a starting state";
            return error(m, r->row);
        }
    }
    return NULL;
}

static int findPattern(strings *ps, char *s) {
    for (int i = 0; i < countStrings(ps); i++) {
        if (strcmp(getString(ps, i), s) == 0) return i;
    }
    return -1;
}

static char *fillActions(states *ss) {
    for (int i = 0; i < countRules(ss->rs); i++) {
        rule *r = getRule(ss->rs, i);
        state *s = findState(ss, r->base);
        int op = findString(ss->types, r->type);
        if (r->lookahead) op = (0x80 | op);
        for (int j = 0; j < countStrings(r->patterns); j++) {
            char *pattern = getString(r->patterns, j);
            int p = findPattern(ss->patterns, pattern);
            if (p < 0) return error("can't find pattern %s\n", pattern);
            if (s->actions[p].op == SKIP) {
                s->actions[p].op = op;
                state *t = findState(ss, r->target);
                s->actions[p].target = t->index;
            }
        }
    }
    return NULL;
}

// Check that each state covers all input characters. Assume each character is
// represented as a one-character pattern.
static char *checkComplete(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        for (int p = 0; p < countStrings(ss->patterns); p++) {
            char *ps = getString(ss->patterns, p);
            if (strlen(ps) != 1) continue;
            if (s->actions[p].op == SKIP) {
                char temp[10];
                if (ps[0] < ' ') {
                    sprintf(temp, "\\%d", ps[0]);
                    ps = temp;
                }
                return error(
                    "state %s has no rule for character '%s'",
                    s->name, ps
                );
            }
        }
    }
    return NULL;
}

// Visit a state during a depth first search, looking for a loop with no
// progress when the given character is next in the input. A lookahead action
// for a pattern starting with the character indicates a progress-free jump to
// another state. Any (non-skip) action for a pattern which is that single
// character indicates that the character has been dealt with. Return true for
// success, false for a loop. (Search backward through patterns so that longer
// ones are checked first.)
static bool visit(states *ss, state *s, char ch) {
    if (s->visited) return true;
    if (s->visiting) return false;
    s->visiting = true;
    for (int i = countStrings(ss->patterns) - 1; i >= 0; i--) {
        char *p = getString(ss->patterns, i);
        if (p[0] > ch) continue;
        if (p[0] < ch) break;
        unsigned int op = s->actions[i].op;
        if (op == SKIP) continue;
        bool lookahead = (op & 0x80) != 0;
        if (lookahead) {
            bool ok = visit(ss, ss->a[s->actions[i].target], ch);
            if (! ok) return false;
        }
        if (strlen(p) == 1) break;
    }
    s->visiting = false;
    s->visited = true;
    return true;
}

// Report a progress-free loop of states when ch is next in the input.
static char *reportLoop(states *ss, char ch) {
    ch = ch & 0x7F;
    char *m = message;
    m += sprintf(m, "Error: possible infinite loop on ");
    if (ch == '\'') m += sprintf(m, "'");
    else if (ch < ' ' || ch == 127) m += sprintf(m, "\\%d", ch);
    else m += sprintf(m, "'%c'", ch);
    m += sprintf(m, " for states:");
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        if (! s->visiting) continue;
        m += sprintf(m, " %s", s->name);
    }
    return message;
}

// For each character, initialise flags and do a depth first search.
static char *checkProgress(states *ss) {
    for (int ch = 0; ch <= 127; ch++) {
        for (int i = 0; i < ss->n; i++) {
            state *s = ss->a[i];
            s->visiting = s->visited = false;
        }
        for (int i = 0; i < ss->n; i++) {
            state *s = ss->a[i];
            bool ok = visit(ss, s, ch & 0x7F);
            if (! ok) return reportLoop(ss, ch & 0x7F);
        }
    }
    return NULL;
}

char *checkAndFillActions(states *ss) {
    if (ss->n > 128) return error("more than 128 states");
    char *m = checkDefined(ss);
    if (m == NULL) m = checkClassification(ss);
    if (m == NULL) m = checkJumps(ss);
    if (m == NULL) m = fillActions(ss);
    if (m == NULL) m = checkComplete(ss);
    if (m == NULL) m = checkProgress(ss);
    return m;
}

static action getAction(states *ss, state *base, char *pattern) {
    int p = findPattern(ss->patterns, pattern);
    return base->actions[p];
}

static char *getType(states *ss, int i) {
    return getString(ss->types, i);
}

static int getIndex(states *ss, char *s) {
    state *base = findState(ss, s);
    return base->index;
}

void writeTable(states *ss, char const *path) {
    FILE *fp = fopen(path, "wb");
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        fprintf(fp, "%s%c", s->name, '\0');
    }
    fprintf(fp, "%c", '\0');
    for (int i = 0; i < countStrings(ss->patterns); i++) {
        char *p = getString(ss->patterns, i);
        fprintf(fp, "%s%c", p, '\0');
    }
    fprintf(fp, "%c", '\0');
    for (int i = 0; i < countStrings(ss->types); i++) {
        char *t = getString(ss->types, i);
        fprintf(fp, "%s%c", t, '\0');
    }
    fprintf(fp, "%c", '\0');
    int np = countStrings(ss->patterns);
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        fwrite(s->actions, 2, np, fp);
    }
    fclose(fp);
}

#ifdef statesTest

// Each example used in testing has a string (made of concatenated lines)
// forming a language description, then strings which test some generated table
// entries (or an error message), then a NULL. Each test checks an entry in the
// generated table, expressed roughly in the original rule format, except that
// each rule has a single pattern and the tag is a single character and an
// explicit . is used to mean continue.

// A basic example.
char *eg1[] = {
    "start == != start OP\n"
    "start start ERROR\n",
    // ----------------------
    "start == start OP",
    "start != start OP",
    "start ? start ERROR", NULL
};

// Rule with no tag, continuing the token.
char *eg2[] = {
    "start 0..9 number\n"
    "start start ERROR\n"
    "number 0..9 start VALUE\n"
    "number start ERROR\n",
    //--------------------------
    "start 0 number",
    "start 5 number",
    "start 9 number",
    "start ? start ERROR",
    "number 5 start VALUE",
    "number ? start ERROR", NULL
};

// Longer pattern takes precedence (= is a prefix of ==). (Can't test here
// whether the patterns are ordered so that == is before =)
char *eg3[] = {
    "start = start SIGN\n"
    "start == != start OP\n"
    "start start ERROR\n",
    //-----------------------
    "start = start SIGN",
    "start == start OP", NULL
};

// Earlier rule for same pattern takes precedence (> is matched by last rule).
char *eg4[] = {
    "start < filename\n"
    "start start ERROR\n"
    "filename > start QUOTE\n"
    "filename filename\n",
    //-------------------------
    "start < filename",
    "filename > start QUOTE",
    "filename ! filename", NULL
};

// A lookahead rule allows a token's type to be affected by what follows.
char *eg5[] = {
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

// A lookahead rule can be a continuing one. It is a jump to another state.
char *eg6[] = {
    "start a start ID\n"
    "start . start2 +\n"
    "start start ERROR\n"
    "start2 . start2 DOT\n"
    "start2 start +\n",
    //-------------------
    "start . start2 +", NULL
};

// Identifier may start with keyword.
char *eg7[] = {
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

// Can have multiple starting states.
char *eg8[] = {
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

// An undefined state.
char *eg9[] = {
    "start == != start OP\n"
    "start unknown ERROR\n",
    // ----------------------
    "Error: undefined state 'unknown' on line 2", NULL
};

// 'start' is a starting state because it is first, but also a continuing state
// because it is the target of a non-terminating non-lookahead rule.
char *eg10[] = {
    "start x start\n"
    "start start ERROR\n",
    //----------------------
    "Error: start is a starting state (line 1) and a continuing state (line 1)",
    NULL
};

// 'start' is a starting state because it is first, but also a continuing state
// because it is the base of the first terminating lookahead rule.
char *eg11[] = {
    "start ; start OP+\n"
    "start start ERROR\n",
    //----------------------
    "Error: start is a starting state (line 1) and a continuing state (line 1)",
    NULL
};

// 'prop' is a starting state because of line 4, and continuing state because of
// lines 6/7.
char *eg12[] = {
    "start . dot\n"
    "start start ERROR\n"
    "dot 0..9 start NUM\n"
    "dot a..z A..Z prop SIGN+\n"
    "dot start ERROR\n"
    "prop a..z A..Z 0..9 prop\n"
    "prop start PROPERTY\n",
    //----------------------
    "Error: prop is a starting state (line 4) and a continuing state (line 6)",
    NULL
};

// The second rule jumps from a starting state to a continuing state.
char *eg13[] = {
    "start . dot\n"
    "start dot +\n"
    "start start ERROR\n"
    "dot start ERROR\n",
    //----------------------
    "Error: line 2 jumps from a starting to a continuing state",
    NULL
};

// The third rule jumps from a continuing state to a starting state.
char *eg14[] = {
    "start . dot\n"
    "start start ERROR\n"
    "dot start +\n",
    //----------------------
    "Error: line 3 jumps from a continuing to a starting state",
    NULL
};

// The 'dot' state is not complete.
char *eg15[] = {
    "start . dot\n"
    "start start ERROR\n"
    "dot \\1..w y..\\127 start DOT\n",
    //----------------------
    "Error: state dot has no rule for character 'x'",
    NULL
};

// 'start' jumps to itself without making progress (a loop on \1 is reported).
char *eg16[] = {
    "start start +\n",
    //----------------------
    "Error: possible infinite loop on \\1 for states: start",
    NULL
};

// 'start' jumps to itself without making progress on input 'x'.
char *eg17[] = {
    "start x start +\n"
    "start start ERROR\n",
    //----------------------
    "Error: possible infinite loop on 'x' for states: start",
    NULL
};

// The states cycle on input 'x'.
char *eg18[] = {
    "start x three +\n"
    "start start ERROR\n"
    "two x start +\n"
    "two start ERROR\n"
    "three x two +\n"
    "three start ERROR\n",
    //----------------------
    "Error: possible infinite loop on 'x' for states: start two three",
    NULL
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
    state *sb = findState(ss, base);
    if (sb == NULL) {
        printf("Test failed: %s: no state '%s'\n", name, base);
        exit(1);
    }
    action act = getAction(ss, sb, pattern);
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
void runExample(char *name, char *eg[]) {
    char text[1000];
    strcpy(text, eg[0]);
    rules *rs = newRules(text);
    states *ss = newStates(rs);
    char *e = checkAndFillActions(ss);
    if (e != NULL) {
        if (strcmp(e, eg[1]) != 0) {
            printf("Test failed: %s: the rules generate error message:\n", name);
            printf("    %s\n", e);
            printf("but the expected error message is:\n");
            printf("    %s\n", eg[1]);
            exit(0);
        }
    }
    else for (int i = 1; eg[i] != NULL; i++) checkAction(ss, name, eg[i]);
    freeStates(ss);
    freeRules(rs);
}

// Run all the tests. Keep the last few commented out during normal operation
// because they test error messages.
void runTests() {
    runExample("eg1", eg1);
    runExample("eg2", eg2);
    runExample("eg3", eg3);
    runExample("eg4", eg4);
    runExample("eg5", eg5);
    runExample("eg6", eg6);
    runExample("eg7", eg7);
    runExample("eg8", eg8);
    runExample("eg9", eg9);
    runExample("eg10", eg10);
    runExample("eg11", eg11);
    runExample("eg12", eg12);
    runExample("eg13", eg13);
    runExample("eg14", eg14);
    runExample("eg15", eg15);
    runExample("eg16", eg16);
    runExample("eg17", eg17);
    runExample("eg18", eg18);
}

int main(int argc, char const *argv[]) {
    runTests();
    return 0;
}

#endif
