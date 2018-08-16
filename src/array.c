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

void *setSize(void *xs, int i, int n) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    assert(0 <= i && i <= a->len);
    if (a->cap < n + 1) {
        while (a->cap < n + 1) a->cap = a->cap * 3 / 2;
        a = realloc(a, sizeof(array) + a->cap * a->stride);
    }
    int m = a->len;
    if (n > m) {
        int from = i * a->stride, to = (i+n-m) * a->stride;
        int amount = (m-i+1) * a->stride;
        memmove(&a->data[to], &a->data[from], amount);
    }
    else if (n < m) {
        int from = (i+n-m) * a->stride, to = i * a->stride;
        int amount = (n-i+1) * a->stride;
        memmove(&a->data[to], &a->data[from], amount);
    }
    a->len = n;
    return a->data;
}

#ifdef test_array

int main() {
    char *s = newArray(sizeof(char));
    s[0] = '\0';
    assert(size(s) == 0);
    s = setSize(s, 0, 1);
    s[0] = 'x';
    assert(strcmp(s, "x") == 0);
    s = setSize(s, 0, 0);
    assert(strcmp(s, "") == 0);
    printf("Array module OK\n");
}

#endif
