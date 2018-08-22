// The Snipe editor is free and open source, see licence.txt.
#include "indent.h"
#include "style.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

// Fix tabs at four spaces for now.
enum { TAB = 4 };

// Reuse the POINT/SELECT flags temporarily as indenter/outdenter markers.
enum { IN = POINT, OUT = SELECT };

// Match two bracket characters.
static bool match(char o, char c) {
    if (o == '{' && c == '}') return true;
    if (o == '[' && c == ']') return true;
    if (o == '(' && c == ')') return true;
    return false;
}

// Implement a stack as an array s of ints with the top index at s[0].
static inline void make(int s[]) { s[0] = 1; }
static inline bool empty(int s[]) { return s[0] == 1; }
static inline void push(int s[], int i) { s[s[0]++] = i; }
static inline int pop(int s[]) { return s[--s[0]]; }
static inline int top(int s[]) { return s[s[0]-1]; }

// Use a variation on the usual stack-based algorithm. An open bracket is
// tentatively marked as an indenter and pushed on the stack. If ...[] is found,
// both brackets are marked as matched. If ] is found on its own, it is marked
// as an outdenter. If ...[(] is found, ( is marked as an error and [] as a
// match. Otherwise, if ...(] is found, ] is marked as an error, and ( is
// tentatively marked as an error and left on the stack. If subsequently
// ...() is found, it becomes a match, but if ...(] is found then ( is
// popped and accepted as an error. At the end, all tentative markings are
// accepted without further work.
static void matchBrackets(int n, char const line[n], char styles[n]) {
    int stack[n+1];
    make(stack);
    for (int i = 0; i < n; i++) {
        if (styles[i] != addStyleFlag(SIGN,START)) continue;
        char c = line[i];
        if (c == '{' || c == '[' || c == '(') {
            styles[i] = addStyleFlag(styles[i], IN);
            push(stack, i);
            continue;
        }
        if (c != '}' && c != ']' && c != ')') continue;
        if (empty(stack)) {
            styles[i] = addStyleFlag(styles[i], OUT);
            continue;
        }
        int j = pop(stack);
        if (match(line[j], c)) {
            styles[j] = styles[i] = addStyleFlag(SIGN,START);
            continue;
        }
        if (styles[j] == addStyleFlag(BAD,START)) {
            i--;
            continue;
        }
        if (!empty(stack) && match(line[top(stack)], c)) {
            styles[j] = addStyleFlag(BAD,START);
            int k = pop(stack);
            styles[k] = styles[i] = addStyleFlag(SIGN,START);
            continue;
        }
        styles[j] = addStyleFlag(BAD,START);
        push(stack,j);
        styles[i] = addStyleFlag(BAD,START);
    }
}

// Count the number of outdenters and indenters, and remove their flags. A line
// contains any number of them. There can't be an outdenter after an indenter,
// because that would cause a mismatch. Each outdenter reduces the indent on the
// current line, and each indenter increases the indent on the following line.
static void countOutIn(int *out, int *in, int n, char styles[n]) {
    *in = 0;
    *out = 0;
    for (int i = 0; i < n; i++) {
        if (hasStyleFlag(styles[i], OUT)) *out = *out + 1;
        else if (hasStyleFlag(styles[i], IN)) *in = *in + 1;
        else continue;
        styles[i] = addStyleFlag(clearStyleFlags(styles[i]), START);
    }
 }

// Match brackets then update the running indent according to the outdenters
// and indenters. Make the indent zero for a blank line.
int findIndent(int *runningIndent, int n, char const line[n], char styles[n]) {
    int indent = *runningIndent;
    int outdenters, indenters, result;
    matchBrackets(n, line, styles);
    countOutIn(&outdenters, &indenters, n, styles);
    indent -= outdenters * TAB;
    if (indent < 0) indent = 0;
    result = indent;
    if (n == 0) result = 0;
    indent += indenters * TAB;
    *runningIndent = indent;
    return result;
}

#ifdef test_indent

static bool checkMatch(char *line, char *out) {
    int n = strlen(line);
    char styles[n+1];
    styles[n] = '\0';
    for (int i=0; i<n; i++) styles[i] = addStyleFlag(SIGN,START);
    matchBrackets(n, line, styles);
    for (int i=0; i<n; i++) {
        if (hasStyleFlag(styles[i], IN)) styles[i] = 'I';
        else if (hasStyleFlag(styles[i], OUT)) styles[i] = 'O';
        else styles[i] = styleLetter(styles[i]);
    }
    bool ok = strcmp(styles, out) == 0;
    return ok;
}

static bool checkIndent(int prev, char *line, int current, int next) {
    int n = strlen(line);
    char styles[n];
    for (int i=0; i<n; i++) styles[i] = addStyleFlag(SIGN,START);
    int indent = findIndent(&prev, n, line, styles);
    return (prev == next) && (indent == current);
}

static void testMatch() {
    assert(checkMatch("...", "XXX"));
    assert(checkMatch("()", "XX"));
    assert(checkMatch("(", "I"));
    assert(checkMatch(")", "O"));
    assert(checkMatch("(]", "BB"));
    assert(checkMatch("](", "OI"));
    assert(checkMatch("[(]", "XBX"));
    assert(checkMatch("[)]", "XBX"));
}

static void testIndent() {
    assert(checkIndent(0, "x", 0, 0));
    assert(checkIndent(0, "f() {", 0, 4));
    assert(checkIndent(4, "}", 0, 0));
    assert(checkIndent(4, "} else {", 0, 4));
}

int main(int n, char const *args[n]) {
    testMatch();
    testIndent();
    printf("Indent module OK\n");
    return 0;
}

#endif

/*
Lookahead
---------
A flag from the following line is useful:
    starts with {                 [continuation => no semicolon]
    OR starts with infix character
Standalone block must be preceded by blank line.

Lookbehind
----------
Items from the previous line are useful [two bytes]
    running indent
    Commenting, Continuing, Comma, Semi, CloseSemi

Scan
----
With the exception of the above, each line is scanned independently.
AFTER scanning, indent and semicolon adjustment is done.
Do not bother trying to synchronize.
On every edit, scan from the 1st line affected to the end of the visible display
Workspace size can be estimated [add indent + tabs + semicolon].
# lines count as one line comments
Can have two kinds of spaces, so cursor moves past autoindents,
    or cursor is adjusted after each movement.

Flags
-----
Commenting at start, end.
Continuing, to be continued.
Comma [inside round or square or init or enum, comma not a continuation]
Semi [inside standalone block or attached block or structure, semi expected]
CloseSemi [inside round or square or structure or init, outdenter needs semi]

Outdent [margin to decrease next line]  =>  end margin
Indent [increase]  =>  end margin
TempOutdent [case, default, label, maybe 2 spaces]  =>  line adjust
TempUndent [blank line]  =>  line adjust
AutoIndent [margin should not be edited = !Commenting]  =>  special spaces

Actual margin = special spaces, after adjustment.
If user presses space/tab just after special space, disallowed, nothing happens.
Otherwise, the cursor goes beyond the end of the line length.
This includes blank lines.
*/
