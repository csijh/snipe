// Snipe language compiler. Free and open source. See licence.txt.
#include "stacks.h"
#include <stdio.h>
#include <stdlib.h>

enum { SIZE = 100 };
struct stacks { int L, R; int ns[SIZE]; };

stacks *newStacks() {
    stacks *ss = malloc(sizeof(stacks));
    ss->L = 0;
    ss->R = SIZE;
    return ss;
}

void pushL(stacks *ss, int i) { ss->ns[ss->L++] = i; }

void pushR(stacks *ss, int i) { ss->ns[--ss->R] = i; }

int topL(stacks *ss) { if (ss->L == 0) return -1; return ss->ns[ss->L - 1]; }

int topR(stacks *ss) { if (ss->R == SIZE) return -1; return ss->ns[ss->R]; }

int popL(stacks *ss) { return ss->ns[--ss->L]; }

int popR(stacks *ss) { return ss->ns[ss->R++]; }

void crash(char const *fmt, ...) {
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

#ifdef stacksTest

int main(int argc, char const *argv[]) {
    return 0;
}

#endif
