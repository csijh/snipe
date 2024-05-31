// The Snipe editor is free and open source. See licence.txt.
#include "scan.h"
#include "array.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Check if a closer matches the top opener.
static bool matchTop(byte *stack, byte closer) {
    int n = length(stack);
    if (n == 0) return false;
    byte opener = stack[n-1];
    return bracketMatch(opener, closer);
}

// Push an opener, assuming the stack is big enough.
static void push(byte *stack, byte opener) {
    adjust(stack, +1);
    stack[length(stack) - 1] = opener;
}

// Pop an opener, and mark the closer as mismatched if appropriate.
static void pop(byte *stack, byte *out, int at) {
    byte closer = out[at];
    byte opener = 0;
    int n = length(stack);
    if (n > 0) {
        opener = stack[n-1];
        adjust(stack, -1);
    }
    if (! bracketMatch(opener, closer)) {
        out[at] = closer | Bad;
    }
}

// Print out one scanning step.
static void trace(char **names, int state, bool look, char *in, int at, int n, int s) {
    char pattern[2*n+2];
    pattern[0] = '\0';
    if (look) strcat(pattern, "|");
    for (int i = 0; i < n; i++) {
        char ch = in[at];
        if (ch == ' ') strcat(pattern, "\\s");
        else if (ch == '\n') strcat(pattern, "\\n");
        else if (ch == '\\') strcat(pattern, "\\\\");
        else if (ch == '|') strcat(pattern, "\\|");
        else strcat(pattern, (char[2]){ch,'\0'});
    }
    char *base = names[state];
    char *style = styleName(s);
    if (s == None) style = "-";
    printf("%-10s %-10s %-10s\n", base, pattern, style);
}

// Given the transition table for a language, and a starting state s0, scan in
// to produce out, using the given stack (assumed big enough), and tracing if
// names is not NULL. Return the final state.
int scan(byte *table, int s0, char *in, byte *out, byte *stack, char **names) {
    int at = 0, start = 0, to = length(in);
    int state = s0;
    while (at < to) {
        char ch = in[at];
        int col = 0;
        if (ch != '\n') col = 1 + (ch - ' ');
        byte *action = &table[CELL * (COLUMNS * state + col)];
        int len = 1;
        if ((action[0] & LINK) != 0) {
            int offset = ((action[0] & 0x7F) << 8) + action[1];
            byte *p = table + offset;
            bool found = false;
            while (! found) {
                found = true;
                len = p[0];
                for (int i = 1; i < len && found; i++) {
                    if (in[at + i] != p[i]) found = false;
                }
                bool look = p[len] & LOOK;
                bool soft = p[len] & SOFT;
                int style = p[len] & ~FLAGS;
                if (found && soft) {
                    if (! look && ! matchTop(stack,style)) found = false;
                    if (look && start == at) found = false;
                }
                if (found) action = p + len;
                else p = p + len + 2;
            }
        }
        bool look = (action[0] & LOOK) != 0;
        int style = action[0] & ~FLAGS;
        int target = action[1];
        if (names != NULL) trace(names, state, look, in, at, len, style);
        if (! look) at = at + len;
        if (style != None && start < at) {
            out[start] = style | First;
            for (int i = start+1; i < at; i++) out[i] = style;
            if (isOpener(style)) push(stack, style);
            else if (isCloser(style)) pop(stack, out, start);
            start = at;
        }
        state = target;
    }
    return state;
}

// ---------- Testing ----------------------------------------------------------
#ifdef scanTest

int main() {
    printf("Scanning is tested in languages/compile.c\n");
}

#endif
