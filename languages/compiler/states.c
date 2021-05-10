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
    int row;
    action *actions;
    bool visiting, visited;
};
typedef struct state state;

enum { SKIP = '~' };

// List of states, plus patterns.
struct states { int c, n; state **a; strings *patterns; };

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

void addState(states *ss, char *name) {
    findState(ss, name);
}

void setType(states *ss, char *name, bool starting) {
    state *s = findState(ss, name);
    s->starting = starting;
}

/*
static bool isStarting(states *ss, char *name) {
    state *s = findState(ss, name);
    return s->starting;
}
*/

void convert(states *ss, int row, char *b, strings *ps, char *t, char tag) {
    state *base = findState(ss, b);
    state *target = findState(ss, t);
    bool differ = (base->starting != target->starting);
    if (countStrings(ps) == 0 && tag == (0x80 | '-') && differ) {
        char *m =
            "Error in rule on line %d\n"
            "states are not both starting or both continuing";
        crash(m, row);
    }
    // TODO
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

void setupActions(states *ss, strings *patterns) {
    ss->patterns = patterns;
}

void fillAction(states *ss, char *name, int p, char tag, char *target) {
    state *s = findState(ss, name);
    int n = countStrings(ss->patterns);
    if (s->actions == NULL) {
        s->actions = malloc(n * sizeof(action));
        for (int p = 0; p < n; p++) s->actions[p].tag = SKIP;
    }
    s->actions[p].tag = tag;
    state *t = findState(ss, target);
    s->actions[p].target = t->index;
}
/*
// Check that all states have rules/actions associated with them.
void checkDefined(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        if (s->actions == NULL) crash("state %s not defined", s->name);
    }
}
*/
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
    states *ss = newStates();
    freeStates(ss);
    return 0;
}

#endif
