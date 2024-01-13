// The Snipe editor is free and open source. See licence.txt.
#include "check.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void crash(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void check(bool ok, char const *fmt, ...) {
    if (ok) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, ".\n");
    exit(EXIT_FAILURE);
}

#ifdef checkTest

int main(int n, char const *args[]) {
    check(true, "Test failed");
    check(false, "Check module OK");
}

#endif
