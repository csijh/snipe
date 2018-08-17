// The Snipe editor is free and open source, see licence.txt.
#include "indent.h"
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

// Match brackets. leave matched brackets as signs. Mark mismatched brackets as
// errors. Flag unmatched brackets as indenters or outdenters.
enum { S='S', SIGN=S, B='B', BAD=B, O='O', OPEN=O, C='C', CLOSE=C };
enum { TAB = 4 };

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
void matchBrackets(int n, char const line[n], char styles[n]) {
    int stack[n+1];
    make(stack);
    for (int i = 0; i < n; i++) {
        if (styles[i] != SIGN) continue;
        char c = line[i];
        if (c == '{' || c == '[' || c == '(') {
            styles[i] = OPEN;
            push(stack, i);
            continue;
        }
        if (c != '}' && c != ']' && c != ')') continue;
        if (empty(stack)) { styles[i] = CLOSE; continue; }
        int j = pop(stack);
        if (match(line[j], c)) {
            styles[j] = styles[i] = SIGN;
            continue;
        }
        if (styles[j] == BAD) {
            i--;
            continue;
        }
        if (!empty(stack) && match(line[top(stack)], c)) {
            styles[j] = BAD;
            int k = pop(stack);
            styles[k] = styles[i] = SIGN;
            continue;
        }
        styles[j] = BAD;
        push(stack,j);
        styles[i] = BAD;
    }
}

// Count the number of indenters and outdenters.
static void countInOut(int *in, int *out, int n, char const styles[n]) {
    *in = 0;
    *out = 0;
    for (int i = 0; i < n; i++) {
        if (styles[i] == CLOSE) *out++;
        else if (styles[i] == OPEN) *in++;
    }
 }

// Change the indent of a non-blank line to the given amount.
static void fixIndent(int indent, string **pline, string **pstyles) {
    string *line = *pline, *styles = *pstyles;
    int old = 0;
    for (int i = 0; i < size(line); i++) if (line[i] == ' ') old++;
    if (old == size(line)) indent = 0;
    line = *pline = reSize(line, 0, indent-old);
    styles = *pstyles = reSize(styles, 0, indent - old);
    for (int i = old; i < indent; i++) {
        line[i] = ' ';
        styles[i] = SIGN;
    }
    // TODO space out initial brackets.
}

// A line contains any number of indenters and outdenters. There can't be an
// outdenter after an indenter, because that would cause a mismatch. There is a
// running indent carried from line to line. Each outdenter reduces the indent
// on the current line, and each indenter increases the indent on the following
// line. Outdenters or indenters at the start of a line are spaced out,
// e.g. ...}...} or ...{...{...text. The flags on brackets are removed.
int autoIndent(int indent, string **linep, string **stylesp) {
    string *line = *linep, *styles = *stylesp;
    int n = size(line);
    int outdenters = 0, indenters = 0;
    for (int i = 0; i < n; i++) {
        if (styles[i] == CLOSE) outdenters++;
        else if (styles[i] == OPEN) indenters++;
    }
    indent -= outdenters * TAB;
    if (indent < 0) indent = 0;
    fixIndent(indent, linep, stylesp);
    line = *linep; styles = *stylesp;
    for (int i = 0; i < n; i++) {
        if (styles[i] == CLOSE || styles[i] == OPEN) styles[i] = SIGN;
    }
    indent += indenters * TAB;
    printf("next indent %d\n", indent);
    return indent;
}

#ifdef test_indent

static bool checkMatch(char *txt, char *out) {
    int n = strlen(txt);
    string *line = newArray(sizeof(char));
    reSize(line, 0, n);
    strcpy(line, txt);
    string *in = newArray(sizeof(char));
    in[0] = '\0';
    reSize(in, 0, n);
    for (int i=0; i<n; i++) in[i] = S;
    matchBrackets(size(line), line, in);
    bool ok = strcmp(in, out) == 0;
    freeArray(in);
    freeArray(line);
    return ok;
}

static void testMatch() {
    assert(checkMatch("...", "SSS"));
    assert(checkMatch("()", "SS"));
    assert(checkMatch("(", "O"));
    assert(checkMatch(")", "C"));
    assert(checkMatch("(]", "BB"));
    assert(checkMatch("](", "CO"));
    assert(checkMatch("[(]", "SBS"));
    assert(checkMatch("[)]", "SBS"));
}

int main(int n, char const *args[n]) {
    testMatch();
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
