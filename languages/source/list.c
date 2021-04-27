// Snipe language compiler. Free and open source. See licence.txt.
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

void crash(char const *fmt, ...) {
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

// Set up an END pointer, similar to NULL, to mark the capacity of a list.
static char end[] = "";
static void *END = end;

// Use generic void * functions, but only internally.
// Generically create a new list of pointers.
static void *newList() {
    int n = 16;
    void **ps = malloc((n + 1) * sizeof(void *));
    ps[0] = NULL;
    ps[n] = END;
    return ps;
}

// Generically find the capacity of a list.
static inline int capacity(void *ls) {
    void **ps = ls;
    int n = 0;
    while (ps[n] != END) n++;
    return n;
}

// Generically find the length of a list.
static inline int length(void *ls) {
    void **ps = ls;
    int n = 0;
    while (ps[n] != NULL) n++;
    return n;
}

// Generically double the capacity of a list.
static void *expand(void *ls) {
    void **ps = ls;
    int n = capacity(ps);
    ps[n] = NULL;
    n = n * 2;
    ps = realloc(ps, (n + 1) * sizeof(void *));
    ps[n] = END;
    return ps;
}

// Generically add an item to a list.
static void add(void *lsp, void *p) {
    if (p == NULL) crash("Can't add NULL item to a list");
    void ***psp = (void ***) lsp;
    void **ps = *psp;
    int n = length(ps);
    if (ps[n+1] == END) *psp = ps = expand(ps);
    ps[n] = p;
    ps[n+1] = NULL;
}

char **newStrings() { return (char **) newList(); }
rule **newRules() { return (rule **) newList(); }
state **newStates() { return (state **) newList(); }
pattern **newPatterns() { return (pattern **) newList(); }
tag **newTags() { return (tag **) newList(); }

int countStrings(char *list[]) { return length(list); }
int countRules(rule *list[]) { return length(list); }
int countStates(state *list[]) { return length(list); }
int countPatterns(pattern *list[]) { return length(list); }
int countTags(tag *list[]) { return length(list); }

void addString(char ***listp, char *s) { add(listp, s); }
void addRule(rule ***listp, rule *r) { add(listp, r); }
void addState(state ***listp, state *s) { add(listp, s); }
void addPattern(pattern ***listp, pattern *p) { add(listp, p); }
void addTag(tag ***listp, tag *t) { add(listp, (void *) t); }
void addQuad(quad ***listp, quad *t) { add(listp, (void *) t); }

#ifdef listTest

int main(int argc, char const *argv[]) {
    char **ss = newStrings();
    char *xs[7] = { "a", "b", "c", "d", "e", "f", "g" };
    for (int i = 0; i < 100; i++) add(&ss, xs[i % 7]);
    assert(countStrings(ss) == 100);
    assert(capacity(ss) == 128);
    assert(ss[0] == xs[0]);
    assert(ss[6] == xs[6]);
    assert(ss[99] == xs[1]);
    free(ss);
    return 0;
}

#endif
