#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

// Support for flexible arrays. The design is a compromise. On the one hand,
// an array needs to be represented by a fixed structure which points to
// variable-sized data, so that the array can be passed to a function which
// updates it without having to pass back the moved data address. On the other
// hand, direct access to the data needs to be provided so that the array
// functions can be fully generic, indexing is convenient and efficient, and
// arrays of structures can be defined.  

// Private array structure. Published as X** for most convenient access.
struct array { char *data; int type, cap, len, size; };
typedef struct array array;

// Pattern for run-time type check.
enum { type = 0x13579BDF };

void *newArray(int size) {
    assert(offsetof(array, data) == 0);
    array *a = malloc(sizeof(array));
    char *data = malloc(8);
    *a = (array) { .type=type, .data=data, .cap=8, .len=0, .size=size };
    return &a->data;
}

void resize(void *xs, int n) {
    array *a = (array *)xs;
    assert(a->type == type);
    a->len = n;
    if (n <= a->cap) return;
    while (a->cap < n) a->cap = a->cap * 3 / 2;
    a->data = realloc(a->data, a->cap * a->size);
}

int main() {
    char **s = newArray(1);
    *s[0] = 'x'; 
    resize(s, 100);
    assert(*s[0] == 'x');
}
