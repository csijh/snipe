// Snipe language compiler. Free and open source. See licence.txt.

// Use generic void * functions, but only internally.
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

void crash(char const *e, int row, char const *s) {
    fprintf(stderr, "Error:");
    fprintf(stderr, " %s", e);
    if (row > 0) fprintf(stderr, " on line %d", row);
    if (strlen(s) > 0) fprintf(stderr, " (%s)", s);
    fprintf(stderr, "\n");
    exit(1);
}

// List structure.
struct list { int capacity, length; void *pointers[]; };
typedef struct list list;

// Generically create a new list of pointers, with capacity and length
static void *newList() {
    int size0 = 16;
    list *ls = malloc(sizeof(list) + size0 * sizeof(void *));
    ls->capacity = size0;
    ls->length = 0;
    return &ls->pointers;
}

// Generically free up a list.
static void freeList(void *ps) {
    list *ls = (list *) ((char *) ps - offsetof(list, pointers));
    free(ls);
}

// Generically find the capacity of a list.
static inline int capacity(void *ps) {
    list *ls = (list *) ((char *) ps - offsetof(list, pointers));
    return ls->capacity;
}

// Generically find the length of a list.
static inline int length(void *ps) {
    list *ls = (list *) ((char *) ps - offsetof(list, pointers));
    return ls->length;
}

// Generically double the capacity of a list.
static void *expand(void *ps) {
    list *ls = (list *) ((char *) ps - offsetof(list, pointers));
    ls->capacity = ls->capacity * 2;
    ls = realloc(ls, sizeof(list) + ls->capacity * sizeof(void *));
    return &ls->pointers;
}

// Generically add an item to a list.
static void *add(void *ps, void *p) {
    if (length(ps) >= capacity(ps)) ps = expand(ps);
    list *ls = (list *) ((char *) ps - offsetof(list, pointers));
    ls->pointers[ls->length] = p;
    ls->length++;
    return &ls->pointers;
}

char **newStrings() { return (char **) newList(); }
rule **newRules() { return (rule **) newList(); }
state **newStates() { return (state **) newList(); }
pattern **newPatterns() { return (pattern **) newList(); }

void freeStrings(char *list[]) { freeList(list); }
void freeRules(rule *list[]) { freeList(list); }
void freeStates(state *list[]) { freeList(list); }
void freePatterns(pattern *list[]) { freeList(list); }

int countStrings(char *list[]) { return length(list); }
int countRules(rule *list[]) { return length(list); }
int countStates(state *list[]) { return length(list); }
int countPatterns(pattern *list[]) { return length(list); }

char **addString(char *list[], char *s) { return (char **) add(list, s); }
rule **addRule(rule *list[], rule *r) { return (rule **) add(list, r); }
state **addState(state *list[], state *s) { return (state **) add(list, s); }
pattern **addPattern(pattern *list[], pattern *p) {
    return (pattern **) add(list, p);
}

#ifdef listTest

int main(int argc, char const *argv[]) {
    char **ss = newStrings();
    char *xs[100] = { "a", "b", "c", "d", "e", "f", "g" };
    xs[99] = "last";
    for (int i = 0; i < 100; i++) ss = add(ss, xs[i]);
    assert(countStrings(ss) == 100);
    assert(capacity(ss) == 128);
    assert(ss[0] == xs[0]);
    assert(ss[6] == xs[6]);
    assert(ss[99] == xs[99]);
    freeStrings(ss);
    return 0;
}

#endif
