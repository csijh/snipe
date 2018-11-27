// The Snipe editor is free and open source, see licence.txt.
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

// An array has a capacity, length, sizeof(item) and items. Externally,
// point to the items.
struct array { int cap, len, stride; char data[]; };
typedef struct array array;

void *newArray(int stride) {
    int cap = 8;
    array *a = malloc(sizeof(array) + (cap * stride));
    *a = (array) { .cap = cap, .len = 0, .stride = stride };
    return a->data;
}

void freeArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    free(a);
}

int size(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    return a->len;
}

void *increase(void *pxs, int i, int n) {
    char **qxs = (char **) pxs;
    array *a = (array *) (((char *) *qxs) - offsetof(array, data));
    assert(0 <= i && i <= a->len && n >= 0);
    int old = a->len, new = a->len + n;
    if (a->cap < new + 1) {
        while (a->cap < new + 1) a->cap = a->cap * 3 / 2;
        a = realloc(a, sizeof(array) + a->cap * a->stride);
    }
    int from = i * a->stride, to = (i+n) * a->stride;
    int amount = (old-i+1) * a->stride;
    memmove(&a->data[to], &a->data[from], amount);
    a->len = new;
    *qxs = a->data;
    return a->data;
}

void decrease(void *xs, int i, int n) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    assert(0 <= i && i <= a->len && 0 <= n && n <= (a->len - i));
    int new = a->len - n;
    int from = (i+n) * a->stride, to = i * a->stride;
    int amount = (new-i+1) * a->stride;
    memmove(&a->data[to], &a->data[from], amount);
    a->len = new;
}

#ifdef arrayTest

int main() {
    char *s = newArray(sizeof(char));
    s[0] = '\0';
    assert(size(s) == 0);
    s = increase(&s, 0, 1);
    s[0] = 'x';
    assert(strcmp(s, "x") == 0);
    decrease(s, 0, 1);
    assert(strcmp(s, "") == 0);
    printf("Array module OK\n");
}

#endif
