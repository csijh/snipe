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

// Compare open and close bracket characters according to priority.
static int match(char o, char c) {
    switch(o) {
      case '{':
        if (c == '}') return 0;
        else return 1;
      case '[':
        if (c == '}') return -1;
        else if (c == ']') return 0;
        return 1;
      case '(':
        if (c == ')') return 0;
        else return -1;
        default: return 0;
    }
}

// Implement a stack as an array s of ints with the top index at s[0].
static inline void make(int s[]) { s[0] = 1; }
static inline bool empty(int s[]) { return s[0] == 1; }
static inline void push(int s[], int i) { s[s[0]++] = i; }
static inline int pop(int s[]) { return s[--s[0]]; }
// static inline int top(int s[]) { return s[s[0]-1]; }

// A conventional stack algorithm is used to mark brackets as matched,
// mismatched, or unmatched (IN or OUT), taking priority into account.

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
        char o = line[j];
        int m = match(o, c);
        if (m == 0) {
            styles[j] = styles[i] = addStyleFlag(SIGN,START);
            continue;
        }
        else if (m < 0) {
            styles[j] = addStyleFlag(BAD,START);
            i--;
            continue;
        }
        else {
            push(stack, j);
            styles[i] = addStyleFlag(BAD,START);
            continue;
        }
    }
}

// TEMPORARILY mark round and square brackets as mismatches, so that
// continuation lines are not yet implemented, and lines aren't moved in and out
// as users type. TODO: match outdenters and indenters with surrounding lines,
// then implement continuation lines properly.

// Count the number of outdenters and indenters, and remove their flags. A line
// contains any number of them. There can't be an outdenter after an indenter,
// because that would cause a mismatch. Each outdenter reduces the indent on the
// current line, and each indenter increases the indent on the following line.
static void countOutIn(
    int *out, int *in, int n, char const line[n], char styles[n]
) {
    *in = 0;
    *out = 0;
    for (int i = 0; i < n; i++) {
        if (hasStyleFlag(styles[i], OUT)) *out = *out + 1;
        else if (hasStyleFlag(styles[i], IN)) *in = *in + 1;
        else continue;
        char ch = line[i];
        if (ch == '(' || ch == '[') {
            styles[i] = addStyleFlag(BAD,START);
            *in = *in - 1;
        }
        else if (ch == ')' || ch == ']') {
            styles[i] = addStyleFlag(BAD,START);
            *out = *out - 1;
        }
        else styles[i] = addStyleFlag(clearStyleFlags(styles[i]), START);
    }
}

int getIndent(int n, char const line[n]) {
    int count = 0;
    for (int i = 0; line[i] == ' '; i++) count++;
    return count;
}

// Match brackets then update the running indent according to the outdenters
// and indenters. Make the indent zero for a blank line. Make the indent 2 less
// for a label line.
int findIndent(int *runningIndent, int n, char const line[n], char styles[n]) {
    int indent = *runningIndent;
    int outdenters, indenters, result;
    matchBrackets(n, line, styles);
    countOutIn(&outdenters, &indenters, n, line, styles);
    int currentIndent = getIndent(n, line);
    indent -= outdenters * TAB;
    if (indent < 0) indent = 0;
    result = indent;
    if (currentIndent == n ||
        (n > 0 && currentIndent == n-1 && line[n-1] == '\n')) {
        result = 0;
    }
    else if (result > 2 && n > 1 && (line[n-1] == ':' || line[n-2] == ':')) {
        result -= 2;
    }
    else {
        char c = line[currentIndent];
        if (c != '}' && c != ']' && c != ')') result += outdenters * TAB;
    }
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
    assert(checkMatch("(]", "BO"));
    assert(checkMatch("](", "OI"));
    assert(checkMatch("[(]", "XBX"));
    assert(checkMatch("[)]", "XBX"));
}

static void testIndent() {
    assert(checkIndent(0, "x", 0, 0));
    assert(checkIndent(0, "f() {", 0, 4));
    assert(checkIndent(4, "}", 0, 0));
    assert(checkIndent(4, "} else {", 0, 4));
    assert(checkIndent(4, "\n", 0, 4));
    assert(getIndent(6, "    x\n") == 4);
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
