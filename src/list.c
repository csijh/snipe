// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct list { char *array; int length, capacity, size; char type; };

ints *newInts() { return newList('I', sizeof(int)); }
int *I(ints *xs) { return A('I', xs); }

chars *newChars() { return newList('C', sizeof(char)); }
char *C(chars *xs) { return A('C', xs); }

strings *newStrings() { return newList('S', sizeof(char *)); }
char **S(strings *xs) { return A('S', xs); }

list *newList(char type, int size) {
    int cap = 12;
    list *xs = malloc(sizeof(list));
    char *a = malloc(cap * size);
    *xs = (list) { .array=a, .length=0, .capacity=cap, .size=size, .type=type };
    return xs;
}

void freeList(list *xs) {
   free(xs->array);
   free(xs);
}

// Switch off assertions for maximum efficiency.
extern inline void *A(char type, list *xs) {
    assert(xs->type == type);
    return xs->array;
}

int length(list *xs) {
    return xs->length;
}

void resize(list *xs, int n) {
    assert(n >= 0);
    xs->length = n;
    if (n > xs->capacity) {
        while (n > xs->capacity) xs->capacity = xs->capacity * 3 / 2;
        xs->array = realloc(xs->array, xs->capacity * xs->size);
    }
}

void expand(list *xs, int i, int n) {
    assert(0 <= i && i <= xs->length && n >= 0);
    resize(xs, xs->length + n);
    int amount = xs->length - n - i;
    char *a = xs->array;
    int s = xs->size;
    if (amount != 0) memmove(a + (i+n)*s, a + i*s, amount*s);
}

void delete(list *xs, int i, int n) {
    assert(0 <= i && n >= 0 && i + n <= xs->length);
    int amount = xs->length - i - n;
    char *a = xs->array;
    int s = xs->size;
    if (amount != 0) memmove(a + i*s, a + (i+n)*s, amount*s);
    xs->length = xs->length - n;
}

#ifdef test_list

int main() {
    setbuf(stdout, NULL);
    ints *xs = newInts();
    resize(xs, 1000);
    assert(length(xs) >= 1000);
    I(xs)[999] = 42;
    assert(I(xs)[999] == 42);
    freeList(xs);
    printf("List module OK\n");
    return 0;
}

#endif
