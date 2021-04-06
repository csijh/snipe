#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

string *newString() {
    struct string *s = malloc(sizeof(string));
    *s = (struct string) { .max = 23, .length = 0, .s = malloc(24) };
    s->s[0] = '\0';
    return s;
}

void freeString(string *s) {
    free(s->s);
    free((struct string *)s);
}

extern inline int lengthString(string *s) {
    return s->length;
}

void setLengthString(string *rs, int n) {
    struct string *s = (struct string *) rs;
    if (s->max < n) {
        while (s->max < n) s->max = s->max * 3/2;
        s->s = realloc(s->s, s->max + 1);
    }
    s->length = n;
    s->s[n] = '\0';
}

void clearString(string *rs) {
    struct string *s = (struct string *) rs;
    if (s->max > 1024) {
        s->max = 23;
        s->s = realloc(s->s, 24);
    }
    s->length = 0;
    s->s[0] = '\0';
}

void fillString(string *rs, char *c) {
    struct string *s = (struct string *) rs;
    setLengthString(rs, strlen(c));
    strcpy(s->s, c);
}

#ifdef stringTest

int main(int n, char const *args[]) {
    string *s = newString();
    assert(s->length == 0 && s->s[0] == '\0');
    setLengthString(s, 4096);
    assert(s->length == 4096 && s->s[4096] == '\0');
    s->s[4096] = '\0';
    freeString(s);
    printf("String module OK\n");
    return 0;
}

#endif
