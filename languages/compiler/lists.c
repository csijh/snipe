// Snipe language compiler. Free and open source. See licence.txt.
#include "lists.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

struct list { int size, max; string **a; };

list *newList() {
    int max0 = 24;
    string **a = malloc(max0 * sizeof(string *));
    list *l = malloc(sizeof(list));
    *l = (list) { .size = 0, .max = max0, .a = a };
    return l;
}

void freeList(list *l, bool contents) {
    if (contents) for (int i = 0; i < l->size; i++) freeString(l->a[i]);
    free(l->a);
    free(l);
}

int size(list *l) {
    return l->size;
}

string *get(list *l, int i) {
    assert(0 <= i && i < l->size);
    return l->a[i];
}

void set(list *l, int i, string *s) {
    assert(0 <= i && i < l->size);
    l->a[i] = s;
}

int add(list *l, string *s) {
    if (l->size >= l->max) {
        l->max = l->max * 3 / 2;
        l->a = realloc(l->a, l->max * sizeof(string *));
    }
    l->a[l->size++] = s;
    return l->size - 1;
}

string *pop(list *l) {
    assert(l->size > 0);
    l->size--;
    return l->a[l->size];
}

void clear(list *l) {
    l->size = 0;
}

int find(list *l, string *s) {
    for (int i = 0; i < l->size; i++) {
        if (compare(s, l->a[i]) == 0) return i;
    }
    return -1;
}

int findOrAdd(list *l, string *s) {
    int i = find(l, s);
    if (i >= 0) return i;
    return add(l, s);
}

list *splitLines(string *s) {
    list *l = newList();
    int p = 0;
    for (int i = 0; i < length(s); i++) {
        if (at(s,i) != '\n') continue;
        add(l, substring(s, p, i));
        p = i + 1;
    }
    return l;
}

list *splitWords(string *s) {
    list *l = newList();
    int start = 0, end = 0, len = length(s);
    while (at(s,start) == ' ') start++;
    while (start < len) {
        end = start;
        while (end < len && at(s,end) != ' ') end++;
        add(l, substring(s, start, end));
        start = end + 1;
        while (start < len && at(s,start) == ' ') start++;
    }
    return l;
}

static int comp(const void *ps1, const void *ps2) {
    const string *s1 = *(string **)ps1;
    const string *s2 = *(string **)ps2;
    return compare(s1, s2);
}

void sort(list *l) {
    qsort(l->a, l->size, sizeof(string *), comp);
}

#ifdef TESTlists

int main() {
    string *text = readFile("lists.c");
    printf("#chars in lists.c = %d\n", length(text));
    list *lines = splitLines(text);
    printf("#lines = %d\n", size(lines));
    list *words = splitWords(get(lines,0));
    printf("#words on first line = %d\n", size(words));
    freeList(words, true);
    freeList(lines, true);
    freeString(text);
    printf("Lists OK\n");
    return 0;
}

#endif
