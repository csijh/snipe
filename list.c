// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

// Generic void pointers are used to avoid duplicated code.
// Compatible specific and generic list structures.
struct chars { int length, capacity; char *items; };
struct ints { int length, capacity; int *items; };
struct strings { int length, capacity; char **items; };
typedef struct chars list;

// Sizes of list items.
enum {
    SC = sizeof(char),
    SI = sizeof(int),
    SS = sizeof(char *)
};

// Generic functions.

static void *newL(int size) {
    int capacity0 = 12;
    list *xs = malloc(sizeof(list));
    char *items = malloc(capacity0 * size);
    *xs = (list) {
        .length = 0, .capacity = capacity0, .items = items
    };
    return xs;
}

static void freeL(void *vxs) {
    list *xs = (list *) vxs;
    free(xs->items);
    free(xs);
}

static void resizeL(void *vxs, int length, int size) {
    list *xs = (list *) vxs;
    xs->length = length;
    if (length <= xs->capacity) return;
    while (length > xs->capacity) xs->capacity = xs->capacity * 3 / 2;
    xs->items = realloc(xs->items, xs->capacity * size);
}

// Add one to the length, and return the index for the new item.
static int addL(void *vxs, int size) {
    list *xs = (list *) vxs;
    int n = xs->length;
    if (n + 1 > xs->capacity) resizeL(xs, n + 1, size);
    xs->length = n + 1;
    return n;
}

static void addListL(void *vxs, void *vys, int size) {
    list *xs = (list *) vxs;
    list *ys = (list *) vys;
    int nx = xs->length, ny = ys->length;
    resizeL(xs, nx + ny, size);
    memcpy(&xs->items[nx * size], &ys->items[0], ny * size);
}

// Insert items into a list from an array.
void insertL(void *vxs, int i, int n, void *a, int size) {
    list *xs = (list *) vxs;
    resizeL(xs, xs->length + n, size);
    char *is = xs->items;
    int amount = xs->length - n - i;
    memmove(is + (i + n) * size, is + i * size, amount * size);
    memcpy(is + i * size, a, n * size);
}

// Delete n items from index i..i+n-1
static void deleteL(void *vxs, int i, int n, int size) {
    list *xs = (list *) vxs;
    int amount = xs->length - i - n;
    if (amount != 0) {
        char *a = xs->items;
        memmove(a + i * size, a + (i + n) * size, amount * size);
    }
    xs->length = xs->length - n;
}

// Extract a sublist
static void sublistL(void *vxs, int p, int n, void *vys, int size) {
    list *xs = (list *) vxs;
    list *ys = (list *) vys;
    resizeL(ys, n, size);
    memcpy(ys->items, &xs->items[p * size], n * size);
}

// Extract a sublist into an array.
static void copyL(void *vxs, int p, int n, void *a, int size) {
    list *xs = (list *) vxs;
    memcpy(a, &xs->items[p * size], n * size);
}

// Compare a sublist with an array.
static bool matchL(void *vxs, int p, int n, void *a, int size) {
    list *xs = (list *) vxs;
    if (xs->length < p + n) return false;
    return strncmp(&xs->items[p * size], a, n * size) == 0;
}

// Convert items to a fixed length array.
static void *freezeL(void *vxs, int size) {
    list *xs = (list *) vxs;
    void *a = realloc(xs->items, xs->length * size);
    free(xs);
    return a;
}

// Specialized functions.

void print(chars *cs) {
    printf("%.*s", cs->length, cs->items);
}

chars *newChars() { return newL(SC); }
ints *newInts() { return newL(SI); }
strings *newStrings() { return newL(SS); }

void freeChars(chars *xs) { freeL(xs); }
void freeInts(ints *xs) { freeL(xs); }
void freeStrings(strings *xs) { freeL(xs); }

int lengthC(chars *xs) { return xs->length; }
int lengthI(ints *xs) { return xs->length; }
int lengthS(strings *xs) { return xs->length; }

extern inline char getC(chars *xs, int i) { return xs->items[i]; }
extern inline int getI(ints *xs, int i) { return xs->items[i]; }
extern inline char *getS(strings *xs, int i) { return xs->items[i]; }

extern inline void setC(chars *xs, int i, char x) { xs->items[i] = x; }
extern inline void setI(ints *xs, int i, int x) { xs->items[i] = x; }
extern inline void setS(strings *xs, int i, char *x) { xs->items[i] = x; }

void addC(chars *xs, char x) { int i = addL(xs, SC); xs->items[i] = x; }
void addI(ints *xs, int x) { int i = addL(xs, SI); xs->items[i] = x; }
void addS(strings *xs, char *x) { int i = addL(xs, SS); xs->items[i] = x; }

void addListC(chars *xs, chars *ys) { addListL(xs, ys, SC); }
void addListI(ints *xs, ints *ys) { addListL(xs, ys, SI); }
void addListS(strings *xs, strings *ys) { addListL(xs, ys, SS); }

void insertC(chars *l, int i, int n, char *a) { insertL(l, i, n, a, SC); }
void insertI(ints *l, int i, int n, int *a) { insertL(l, i, n, a, SI); }
void insertS(strings *l, int i, int n, char **a) { insertL(l, i, n, a, SS); }

void deleteC(chars *xs, int i, int n) { deleteL(xs, i, n, SC); }
void deleteI(ints *xs, int i, int n) { deleteL(xs, i, n, SI); }
void deleteS(strings *xs, int i, int n) { deleteL(xs, i, n, SS); }

void copyC(chars *l, int p, int n, char *a) { copyL(l, p, n, a, SC); }
void copyI(ints *l, int p, int n, int *a) { copyL(l, p, n, a, SI); }
void copyS(strings *l, int p, int n, char **a) { copyL(l, p, n, a, SS); }

void sublistC(chars *l, int p, int n, chars *s) { sublistL(l, p, n, s, SC); }
void sublistI(ints *l, int p, int n, ints *s) { sublistL(l, p, n, s, SI); }
void sublistS(strings *l, int p, int n, strings *s) { sublistL(l,p,n,s,SS); }

bool matchC(chars *l, int p, int n, char *a) { return matchL(l, p, n, a, SC); }
bool matchI(ints *l, int p, int n, int *a) { return matchL(l, p, n, a, SI); }
bool matchS(strings *l, int p, int n, char **a) { return matchL(l,p,n,a,SS); }

void resizeC(chars *xs, int n) { resizeL(xs, n, SC); }
void resizeI(ints *xs, int n) { resizeL(xs, n, SI); }
void resizeS(strings *xs, int n) { resizeL(xs, n, SS); }

char *freezeC(chars *xs) { return freezeL(xs, SC); }
int *freezeI(ints *xs) { return freezeL(xs, SI); }
char **freezeS(strings *xs) { return freezeL(xs, SS); }

#ifdef test_list

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    strings *xs = newStrings();
    resizeS(xs, 1000);
    assert(xs->capacity == 1021);
    freeStrings(xs);
    printf("List module OK\n");
    return 0;
}

#endif
