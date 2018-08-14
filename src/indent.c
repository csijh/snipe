#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

// Match brackets. Mark mismatched brackets as errors, and unmatched brackets as
// openers or closers. Mark only one bracket as mismatched where reasonable,
// e.g. mark only the middle bracket as mismatched in "[(]" and "[)]".

// TODO: allow multiple opener/closers on a line.
// TODO: do the Horstmann indenting after an open bracket starter.

enum { S='S', SIGN=S, B='B', BAD=B, O='O', OPEN=O, C='C', CLOSE=C };

// Match two bracket characters.
static bool match(char o, char c) {
    if (o == '{' && c == '}') return true;
    if (o == '[' && c == ']') return true;
    if (o == '(' && c == ')') return true;
    return false;
}

// Match brackets. Only brackets marked SIGN are recognized, others are assumed
// to be inside comments or strings. After matching, matched brackets remain
// marked SIGN, mismatched brackets are marked BAD, and unmatched brackets are
// marked OPEN or CLOSE. Attempt to mark only one of a mismatched pair as BAD.
void matchBrackets(int n, char const line[n], char styles[n]) {
    char stack[n];
    int top = 0;
    for (int i = 0; i < n; i++) {
        if (styles[i] != SIGN) continue;
        char c = line[i];
        if (c == '{' || c == '[' || c == '(') {
            styles[i] = OPEN;
            stack[top++] = i;
            continue;
        }
        if (c != '}' && c != ']' && c != ')') continue;
        if (top == 0) { styles[i] = CLOSE; continue; }
        int j = stack[--top];
        if (match(line[j], c)) {
            styles[j] = styles[i] = SIGN;
            continue;
        }
        if (styles[j] == BAD) {
            i--;
            continue;
        }
        if (top > 0 && match(line[stack[top-1]],c)) {
            styles[j] = BAD;
            int k = stack[--top];
            styles[k] = styles[i] = SIGN;
            continue;
        }
        styles[j] = BAD;  // tentative
        stack[top++] = j; // put it back
        styles[i] = BAD;
    }
}

/*
int findOpeners(int n, char line[n], char styles[n]) {
    int openerCurly = -1, openerSquare = -1, openerRound = -1;
    for (int i = 0; i < strlen(line); i++) {
        char ch = line[i];
        switch (ch) {
            case '{': openerCurly = i; break;
            case '}': openerCurly = -1; break;
            case '[': openerSquare = i; break;
            case ']': openerSquare = -1; break;
            case '(': openerRound = i; break;
            case ')': openerRound = -1; break;
        }
    }
    int opener = openerCurly;
    if (openerSquare > opener) opener = openerSquare;
    if (openerRound > opener) opener = openerRound;
    return opener;
}

int findCloser(char *line) {
    int closerCurly = INT_MAX, closerSquare = INT_MAX, closerRound = INT_MAX;
    for (int i = strlen(line) - 1; i >= 0; i--) {
        char ch = line[i];
        switch (ch) {
            case '{': closerCurly = -1; break;
            case '}': closerCurly = i; break;
            case '[': closerSquare = -1; break;
            case ']': closerSquare = i; break;
            case '(': closerRound = -1; break;
            case ')': closerRound = i; break;
        }
    }
    int closer = closerCurly;
    if (closerSquare < closer) closer = closerSquare;
    if (closerRound < closer) closer = closerRound;
    if (closer == INT_MAX) closer = -1;
    return closer;
}

int show(char *line, int indent, int cl, int op, FILE *out) {
    int margin = indent;
    if (cl == 0 && margin >= 4) margin = margin - 4;
    for (int i=0; i<margin; i++) fprintf(out, " ");
    fprintf(out, "%s", line);
    if (cl >= 0 && indent >= 4) indent -= 4;
    if (op >= 0) indent += 4;
    return indent;
}
*/

static bool checkMatch(char const *line, char const *out) {
    int n = strlen(line);
    char in[n + 1];
    for (int i=0; i<n; i++) in[i] = S;
    in[n] = '\0';
    matchBrackets(n, line, in);
    return strcmp(in, out) == 0;
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
    /*
    FILE *in = fopen(args[1], "r");
    FILE *out = fopen(args[2], "w");
    fseek(in, 0L, SEEK_END);
    int size = ftell(in);
    fseek(in, 0L, SEEK_SET);
    char *text = malloc(size);
    fread(text, 1, size, in);
    char *styles = malloc(size);
    for (int i=0; i<size; i++) styles[i] = S;
    int start = 0;
    for (int end = 0; end < size; end++) {
        if (text[end] == '\n') {
            text[end] = '\0';
            styles[end] = '\0';
            matchBrackets(end - start, &text[start], &styles[start]);
            printf("%s\n%s\n", &text[start], &styles[start]);
            start = end + 1;
        }
    }
    */
/*
    char *line;
    size_t len;
    ssize_t result;
    int indent = 0;
    result = readline(&line, &len, in);
    while (result != -1) {
        char styles[len+1];
        for (int i=0; i<len; i++) styles[i] = S;
        styles[len] = '\0';
        matchBrackets(len, line, styles);
        printf("%s\n%s\n", line, styles);
*/
/*
        while (line[0] == ' ') line++;
        int op = findOpener(line), cl = findCloser(line);
        if (cl >= 0 && op >= 0 && cl > op) cl = op = -1;
        indent = show(line, indent, cl, op, out);
*/
/*
        result = getline(&line, &len, in);
    }
*/
/*
    fclose(in);
    fclose(out);
    */
    return 0;
}

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
Workspace size can be estimated [add indent + tab + semicolon].
# lines count as one line comments
Can have two kinds of spaces, so cursor moves past autoindents,
    or cursor is adjusted after each movement.

Flags
-----
Commenting at start, end.
Continuing, to be continued.
Comma [inside round or square or init or enum, comma not a continuation]
Semi [inside standalone block or attached block or structure, semi expected]
CloseSemi [inside round or square or structure or init, closer needs semi]

Outdent [margin to decrease next line]  =>  end margin
Indent [increase]  =>  end margin
TempOutdent [case, default, label, maybe 2 spaces]  =>  line adjust
TempUndent [blank line]  =>  line adjust
AutoIndent [margin should not be edited = !Commenting]  =>  special spaces

Actual margin = special spaces, after adjustment.
If user presses space/tab just after special space, disallowed, nothing happens.
Otherwise, the cursor goes beyond the end of the line length.
This includes blank lines.

Indent rules
------------
After scanning, look at brackets.
An opener is an open bracket not followed by any brackets of the same kind.
Only the last opener on a line counts.
Similarly closer.
A closer with a later opener is OK.
If the opener is followed by the closer, neither count.
Remaining unmatched brackets are marked.

An opener indents on the next line [Horstmann: user handles first indent]
A closer outdents, on the same line at start, next line otherwise.
First continuation line outdents.
*/
