// Snipe language compiler. Free and open source. See licence.txt.
#include "states.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// An action contains a tag and a target state, each stored in one byte. The tag
// may have the top bit set to indicate a lookahead.
struct action { unsigned char tag, target; };
typedef struct action action;

// A state stores the line number on which its starting flag was established, or
// 0 if not established yet, to allow a sensible error message for a clash.
struct state {
    char *name;
    int index;
    bool starting;
    action *actions;
    bool visiting, visited;
};
typedef struct state state;

enum { SKIP = '~' };

// Lists of rules and patterns, and array of states.
struct states { rules *rs; strings *patterns; int n; state *a[]; };

static void freeState(state *s) {
    if (s->actions != NULL) free(s->actions);
    free(s);
}

// Find the distinct names of base states.
static void findNames(rules *rs, strings *names) {
    for (int i = 0; i < countRules(rs); i++) {
        rule *r = getRule(rs, i);
        char *name = r->base;
        bool found = false;
        for (int j = 0; j < countStrings(names); j++) {
            char *s = getString(names, j);
            if (strcmp(s, name) == 0) found = true;
        }
        if (! found) addString(names, name);
    }
}

// Find a state by name.
static state *findState(states *ss, char *name) {
    for (int i = 0; i < ss->n; i++) {
        if (strcmp(name, ss->a[i]->name) == 0) return ss->a[i];
    }
    return NULL;
}

// For each target state, check whether it is defined.
static void checkDefined(states *ss, rules *rs) {
    for (int i = 0; i < countRules(rs); i++) {
        rule *r = getRule(rs, i);
        char *name = r->target;
        state *s = findState(ss, name);
        if (s == NULL) crash("undefined state %s on line %d", name, r->row);
    }
}

states *newStates(rules *rs) {
    strings *names = newStrings();
    findNames(rs, names);
    int n = countStrings(names);
    states *ss = malloc(sizeof(states) + n * sizeof(state *));
    *ss = (states) { .n = n, .patterns = getPatterns(rs), .rs = rs };
    for (int i = 0; i < countStrings(names); i++) {
        char *name = getString(names, i);
        state *s = malloc(sizeof(state));
        *s = (state) { .name = name, .actions = NULL };
        ss->a[i] = s;
    }
    checkDefined(ss, rs);
    return ss;
}

void freeStates(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        freeState(ss->a[i]);
    }
    free(ss->a);
    free(ss);
}

// Check if a rule terminates the current token.
static bool isTerminating(rule *r) {
    return strcmp(r->tag, "-") == 0;
}

static bool isStarting(rules *rs, char *state) {
    int startingRow = 0, continuingRow = 0;
    rule *r0 = getRule(rs, 0);
    if (strcmp(state, r0->base) == 0) startingRow = r0->row;
    for (int i = 0; i < countRules(rs); i++) {
        rule *r = getRule(rs, i);
        if (strcmp(state, r->base) == 0) {
            if (i == 0) startingRow = r->row;
            if (r->lookahead && isTerminating(r)) continuingRow = r->row;
        }
        else if (strcmp(state, r->target) == 0) {
            if (isTerminating(r)) startingRow = r->row;
            else if (! r->lookahead) continuingRow = r->row;
        }
    }
    if (startingRow > 0 && continuingRow > 0) {
        char *message =
            "Error: %s is a starting state (line %d) "
            "and a continuing state (line %d)";
        crash(message, state, startingRow, continuingRow);
    }
    return (continuingRow == 0);
}

void checkTypes(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        s->starting = isStarting(ss->rs, s->name);
    }
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

static void checkLookahead(states *ss) {
    for (int i = 0; i < countRules(ss->rs); i++) {
        rule *r = getRule(ss->rs, i);
        state *base = findState(ss, r->base);
        state *target = findState(ss, r->target);
        bool differ = (base->starting != target->starting);
        if (countStrings(r->patterns) == 0 && r->lookahead && differ) {
            char *m = (
                "Error in rule on line %d\n"
                "states are not both starting or both continuing"
            );
            crash(m, r->row);
        }
    }
}

static int findPattern(strings *ps, char *s) {
    for (int i = 0; i < countStrings(ps); i++) {
        if (strcmp(getString(ps, i), s) == 0) return i;
    }
    return -1;
}

void fillActions(states *ss) {
    checkLookahead(ss);
    for (int i = 0; i < countRules(ss->rs); i++) {
        rule *r = getRule(ss->rs, i);
        state *s = findState(ss, r->base);
        int n = countStrings(ss->patterns);
        if (s->actions == NULL) {
            s->actions = malloc(n * sizeof(action));
            for (int p = 0; p < n; p++) s->actions[p].tag = SKIP;
        }
        int tag = r->tag[0];
        if (r->lookahead) tag = (0x80 | tag);
        for (int j = 0; j < countStrings(r->patterns); j++) {
            int p = findPattern(ss->patterns, getString(r->patterns, j));
            s->actions[p].tag = tag;
            state *t = findState(ss, r->target);
            s->actions[p].target = t->index;
        }
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
            if (s->actions[p].tag == SKIP) {
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
        if (tag == SKIP) continue;
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
void checkProgress(states *ss) {
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
    int np = countStrings(ss->patterns);
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        fwrite(s->actions, 2, np, fp);
    }
    fclose(fp);
}

#ifdef statesTest

int main(int argc, char const *argv[]) {
    /*
    strings *names = newStrings();
    addString(names, "s1");
    addString(names, "s2");
    states *ss = newStates(names);
    freeStates(ss);
    freeStrings(names);
    */
    return 0;
}

#endif
