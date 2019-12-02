// Flexible arrays. Free and open source. See licence.txt.
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

// An array prefix has a capacity, length, and stride i.e. the size of each
// item. When it represents an edit such as an insertion, 'op' gives the
// operation to perform and 'at' gives its position in the text.

// substring such as an insertion or deletion, 'at'
// is the position in the text. The data is given a pointer type, just to make
// sure it is always pointer-aligned.
struct array { length max, length, at; short stride, op; char *data[]; };
typedef struct array array;

void *newArray(length stride) {
    int max = 8;
    array *a = malloc(sizeof(array) + (max * stride));
    *a = (array) { .max = max, .length = 0, .stride = stride, .at = 0 };
    return a->data;
}

void freeArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    free(a);
}

length lengthArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    return a->length;
}

void *resizeArray(void *xs, length n) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    if (a->max < n + 1) {
        while (a->max < n + 1) a->max = a->max * 3/2;
        a = realloc(a, sizeof(array) + a->max * a->stride);
    }
    a->length = n;
    return a->data;
}

void clearArray(void *xs) {
    resizeArray(xs, 0);
}

void placeArray(void *xs, length at) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    a->at = at;
}

length atArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    return a->at;
}

static length maxArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    return a->max;
}

#ifdef arrayTest

int main() {
    char *s = newArray(sizeof(char));
    assert(lengthArray(s) == 0 && maxArray(s) >= 1);
    s = resizeArray(s, 1000);
    assert(lengthArray(s) == 1000 && maxArray(s) >= 1001);
    s[1000] = '\0';
    clearArray(s);
    assert(lengthArray(s) == 0);
    freeArray(s);
    printf("Array module OK\n");
}

#endif
