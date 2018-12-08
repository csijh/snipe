#include <stdio.h>
#include <stdlib.h>

struct list { int stride, capacity, length; int * restrict p; };
typedef struct list list;

int main() {
    printf("%d\n", (int)sizeof(list));
    struct list *xs = malloc(sizeof(list));
    int n;
    {
        xs->p = &n;
        *(xs->p) = 42;
        printf("%d\n", n);
    }
}
