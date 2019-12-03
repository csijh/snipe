// Flexible arrays. Free and open source. See licence.txt.
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

// An array prefix has a capacity, length, and stride i.e. the size of each
// item. When it represents an edit such as an insertion, 'op' gives the
// operation to perform and 'to' gives its target in the text. The data is
// given a pointer type, just to make sure it is always pointer-aligned.
struct array { int max, length, to; short stride, op; char *data[]; };
typedef struct array array;

void *newArray(int stride) {
    int max = 8;
    array *a = malloc(sizeof(array) + (max * stride));
    *a = (array) { .max = max, .length = 0, .stride = stride, .to = 0 };
    return a->data;
}

void freeArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    free(a);
}

int lengthArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    return a->length;
}

void *resizeArray(void *xs, int n) {
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

void setToArray(void *xs, int to) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    a->to = to;
}

int toArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    return a->to;
}

int opArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    return a->op;
}

void setOpArray(void *xs, int op) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    a->op = op;
}

string *newString() {
    string *s = newArray(1);
    s[0] = '\0';
    return s;
}

void freeString(string *s) { freeArray(s); }

string *fillString(string *str, char *s) {
    resizeString(str, strlen(s));
    strcpy(str, s);
    return str;
}

int lengthString(string *s) { return lengthArray(s); }

void *resizeString(string *s, int n) {
    s = resizeArray(s, n);
    s[n] = '\0';
    return s;
}

void clearString(string *s) {
    clearArray(s);
    s[0] = '\0';
}

int toString(string *s) { return toArray(s); }

void setToString(string *s, int to) { setToArray(s, to); }

int opString(string *s) { return opArray(s); }

void setOpString(string *s, int op) { setOpArray(s, op); }

#ifdef arrayTest

int maxArray(void *xs) {
    array *a = (array *) (((char *) xs) - offsetof(array, data));
    return a->max;
}

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
