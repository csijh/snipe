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
        addOrFind(names, name);
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
    *ss = (states) { .n = n, .patterns = getPatterns(rs), .rs = rs };
    for (int i = 0; i < countStrings(names); i++) {
        char *name = getString(names, i);
        state *s = malloc(sizeof(state));
        *s = (state) { .name = name, .actions = NULL };
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
    return r->type != NULL;
}

// For each target state, check whether it is defined.
static char *checkDefined(states *ss) {
    for (int i = 0; i < countRules(ss->rs); i++) {
        rule *r = getRule(ss->rs, i);
        char *name = r->target;
        state *s = findState(ss, name);
        if (s != NULL) continue;
        return error("undefined state %s on line %d", name, r->row);
    }
    return NULL;
}

static char *isStarting(state *s, rules *rs) {
    int startingRow = 0, continuingRow = 0;
    rule *r0 = getRule(rs, 0);
    if (strcmp(s->name, r0->base) == 0) startingRow = r0->row;
    for (int i = 0; i < countRules(rs); i++) {
        rule *r = getRule(rs, i);
        if (strcmp(s->name, r->base) == 0) {
            if (i == 0) startingRow = r->row;
            if (r->lookahead && isTerminating(r)) continuingRow = r->row;
        }
        else if (strcmp(s->name, r->target) == 0) {
            if (isTerminating(r)) startingRow = r->row;
            else if (! r->lookahead) continuingRow = r->row;
        }
    }
    if (startingRow > 0 && continuingRow > 0) {
        char *message =
            "%s is a starting state (line %d) "
            "and a continuing state (line %d)";
        return error(message, s->name, startingRow, continuingRow);
    }
    s->starting = (continuingRow == 0);
    return NULL;
}

static char *checkTypes(states *ss) {
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        char *m = isStarting(s, ss->rs);
        if (m != NULL) return m;
    }
    return NULL;
}

static char *checkLookahead(states *ss) {
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
        if (s->actions == NULL) {
            int n = countStrings(ss->patterns);
            s->actions = malloc(n * sizeof(action));
            for (int p = 0; p < n; p++) s->actions[p].op = SKIP;
        }
        int op = r->type == NULL ? '.' : r->type[0];
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
                if ((ps[0] & 0x7F) < ' ') {
                    sprintf(temp, "\\%d", (ps[0] & 0x7F));
                    ps = temp;
                }
                return error(
                    "state %s has no rule for character '%s' p=%d %s",
                    s->name, ps, p, ps
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
// success, false for a loop.
static bool visit(states *ss, state *s, char ch) {
    if (s->visited) return true;
    if (s->visiting) return false;
    s->visiting = true;
    for (int i = 0; i < countStrings(ss->patterns); i++) {
        char *p = getString(ss->patterns, i);
        if (p[0] < ch) continue;
        if (p[0] > ch) break;
        unsigned int op = s->actions[i].op;
        if (op == SKIP) continue;
        bool lookahead = (op & 0x80) != 0;
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
static char *reportLoop(states *ss, char ch) {
    char *m = message;
    m += sprintf(m, "Error: possible infinite loop with no progress\n");
    m += sprintf(m, "when character '");
    if (ch < ' ' || ch == 127) m += sprintf(m, "\\%d", ch);
    else m += sprintf(m, "%c", ch);
    m += sprintf(m, "' is next in the input.\n");
    m += sprintf(m, "The states involved are:");
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        if (! s->visiting) continue;
        m += sprintf(m, " %s", s->name);
    }
    m += sprintf(m, "\n");
    return m;
}

// For each character, initialise flags and do a depth first search.
static char *checkProgress(states *ss) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if (ch > '\n' && ch < ' ') continue;
        for (int i = 1; i < ss->n; i++) {
            state *s = ss->a[i];
            s->visiting = s->visited = false;
        }
        for (int i = 0; i < ss->n; i++) {
            state *s = ss->a[i];
            bool ok = visit(ss, s, ch);
            if (! ok) return reportLoop(ss, ch);
        }
    }
    return NULL;
}

char *checkAndFillActions(states *ss) {
    if (ss->n > 128) return error("more than 128 states");
    char *m = checkDefined(ss);
    if (m == NULL) m = checkTypes(ss);
    if (m == NULL) m = checkLookahead(ss);
    if (m == NULL) m = fillActions(ss);
    if (m == NULL) m = checkComplete(ss);
    if (m == NULL) m = checkProgress(ss);
    return m;
}

action getAction(states *ss, char *s, char *pattern) {
    state *base = findState(ss, s);
    int p = findPattern(ss->patterns, pattern);
    return base->actions[p];
}

int getIndex(states *ss, char *s) {
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
    int np = countStrings(ss->patterns);
    for (int i = 0; i < ss->n; i++) {
        state *s = ss->a[i];
        fwrite(s->actions, 2, np, fp);
    }
    fclose(fp);
}

#ifdef statesTest

int main(int argc, char const *argv[]) {
    char *m = error("s=%s, n=%d", "abc", 42);
    printf("[%s]\n", m);
    return 0;
}

#endif
