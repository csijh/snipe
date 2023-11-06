// Free and open source, see licence.txt.
// Provide a standalone scanner to test language definitions.
#include "style.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// TODO notes on tags.
// Flag to indicate cursor position?
// Flag to indicate change of background colour?
// Flag to indicate mismatch?
// Sign, PreSign, InSign, PostSign
// Op, PreOp, InOp, PostOp  (++ OP)
// Fixity of brackets? (Begin0/End0 are infix? JSP? Flexible? InBegin/InEnd?)
// Transfer of { type to } type (reversibly)?
// Rescan a line, repair bracket matching, repair indent, repair semicolon.
// Active newline versus commented newline? Or implied by scan state?

// ---------- Lists ------------------------------------------------------------
// Lists are flexible arrays with built-in length and capacity.

// Prefix in front of a list. Use a 'magic' number as a dynamic type check.
struct header { int size, max, unit, magic; };
typedef struct header header;
enum { MAGIC = 0x12345678 };

// Find a list's header, and check that we have a list.
header* head(void *a) {
    header *p = ((header *)a) - 1;
    if (p->magic == MAGIC) return p;
    fprintf(stderr, "Error: list operation on a non-list\n");
    exit(1);
}

// Allocate an empty list with a given item-size.
void *allocate(int unit) {
    int max = (unit < 8) ? (8 / unit) : 1;
    int total = sizeof(header) + max * unit;
    header *p = malloc(total);
    *p = (header) {.size=0, .max=max, .unit=unit, .magic=MAGIC };
    return p + 1;
}

int size(void *a) { return head(a)->size; }

// Change the size of a list, returning the possibly reallocated list.
void *resize(void *a, int n) {
    header *p = head(a);
    if (n > p->max) {
        while (n > p->max) p->max = 2 * p->max;
        int total = sizeof(header) + p->max * p->unit;
        p = realloc(p, total);
    }
    p->size = n;
    return p + 1;
}

// Increase or decrease list size by d. Return the possibly reallocated list.
void *resizeBy(void *a, int d) { return resize(a, size(a) + d); }

// Make sure there is room for d more items.
void *ensure(void *a, int d) { a = resizeBy(a,d); resizeBy(a,-d); return a; }

// Release memory for a list.
void release(void *a) { free(head(a)); }

// ---------- Rows -------------------------------------------------------------
// A row is a line of text, split into a list of tokens. Read in a language
// description and split it into a list of rows.

// Read a file as a string. Ignore errors. Normalize line endings.
char *readFile(char const *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data =  malloc(size + 1);
    fread(data, 1, size, file);
    data[size] = '\0';
    fclose(file);
    for (int i = 0; data[i] != '\0'; i++) {
        if (data[i] != '\r') continue;
        if (data[i+1] == '\n') data[i] = ' ';
        data[i] = '\n';
    }
    return data;
}

void error(char *format, ...) {
    va_list args;
    fprintf(stderr, "Error: ");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, ".\n");
    exit(1);
}

// Check a line for illegal characters.
void check(int n, char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if ((s[i] & 0x80) != 0) error("non-ascii character on line %d", n);
        if (s[i] < ' ' || s[i] > '~') error("control character on line %d", n);
    }
}

// Get rid of leading, trailing and multiple spaces from a line.
void despace(char *s) {
    int j = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != ' ') s[j++] = s[i];
        else if (j > 0 && s[j-1] != ' ') s[j++] = s[i];
    }
    while (j > 0 && s[j-1] == ' ') j--;
    s[j] = '\0';
}

// Split a string in place into a list of normalized lines.
char **splitLines(char *s) {
    int n = 0;
    for (int i = 0; s[i] != '\0'; i++) if (s[i] == '\n') n++;
    n++;
    char **lines = allocate(sizeof(char *));
    lines = resize(lines, n);
    n = 0;
    lines[n] = &s[0];
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != '\n') continue;
        s[i] = '\0';
        n++;
        lines[n] = &s[i+1];
    }
    n++;
    for (int i = 0; i < n; i++) {
        check (i+1, lines[i]);
        despace(lines[i]);
    }
    return lines;
}

// Check if a token is a state name or a tag name. 
bool isStateName(char *s) { return islower(s[0]); }
bool isTagName(char *s) { return isupper(s[0]); }

// Split a line in place into a list of tokens, if it is a rule. Add a final
// "+" if the rule has no tag.
char **splitTokens(char *s) {
    int n = 0;
    char **tokens = allocate(sizeof(char *));
    if (! islower(s[0])) {
        tokens = resizeBy(tokens, 1);
        tokens[0] = s;
        return tokens;
    }
    n = 0;
    int start = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != ' ') continue;
        s[i] = '\0';
        tokens = resizeBy(tokens, 1);
        tokens[n++] = &s[start];
        start = i + 1;
    }
    tokens = resizeBy(tokens, 1);
    tokens[n++] = &s[start];
    if (isStateName(tokens[0]) && ! isTagName(tokens[n-1])) {
        tokens = resizeBy(tokens, 1);
        tokens[n++] = "+";
    }
    return tokens;
}

// Convert a list of lines into a list of rows.
char ***makeRows(char **lines) {
    int n = size(lines);
    char ***rows = allocate(sizeof(char **));
    rows = resize(rows, n);
    for (int i = 0; i < n; i++) {
        rows[i] = splitTokens(lines[i]);
    }
    return rows;
}

// ---------- States and patterns ----------------------------------------------
// Convert the rules in the language description into a list of states.

// A pattern is a string to be matched, and the action to take on matching it.
struct pattern { char *s; bool lookahead; char tag; int target; };
typedef struct pattern pattern;

// A state has a name, an array of lists of patterns indexed by first character,
// a temporary list of range patterns, and flags to say whether the state can
// occur at the start, or can occur after the start, of a token. There is also
// a visited flag to help check for cycles of states.
struct state {
    char *name; pattern **patterns; pattern *ranges;
    bool starter, adder, visited;
};
typedef struct state state;

// Check a string to see if it is a range of the form x..y 
bool isRange(char *s) {
    if (strlen(s) != 4) return false;
    if (s[1] != '.' || s[2] != '.') return false;
    return true;    
}

// Find an existing state by name, returning its index or -1.
int findState(state *states, char *name) {
    int n = size(states);
    for (int i = 0; i < n; i++) {
        if (strcmp(states[i].name, name) == 0) return i;
    }
    return -1;    
}

// Add a new blank state with the given name, returning the updated list.  
state *addState(state *states, char *name) {
    int n = size(states);
    pattern **patterns = allocate(sizeof(pattern *));
    patterns = resize(patterns, 128);
    for (int i = 0; i < 128; i++) patterns[i] = allocate(sizeof(pattern));
    pattern *ranges = allocate(sizeof(pattern));
    states = resizeBy(states, 1);
    states[n] = (state) {
        .name=name, .patterns=patterns, .ranges=ranges,
        .starter=false, .adder=false, .ender=false
    };
    return states;
}

// Convert a string, target and tag to a pattern. Take off a backslash
// indicating a lookahead, and convert a double backslash into a single.
// Reduce a tag to a single character.
void convert(char *s, int target, char *tag, pattern *p) {
    p->target = target;
    if (tag[0] == 'B') {
        p->tag = tag[strlen(tag)-1];
    }
    else if (tag[0] == 'E') {
        p->tag = 'a' - '0' + tag[strlen(tag)-1];
    }
    else p->tag = tag[0];
    p->lookahead = false;
    if (s[0] == '\\' && (s[1] != '\\' || s[2] == '\\')) {
        p->lookahead = true;
        s = &s[1];
        if (strcmp(s, "s") == 0) s[0] = ' ';
        if (strcmp(s, "n") == 0) s[0] = '\n';
    }
    if (s[0] == '\\' && s[1] == '\\') s = &s[1];
    p->s = s;
}

// Create empty base states from the rules.
state *makeStates(char ***rows) {
    state *states = allocate(sizeof(state));
    for (int i = 0; i < size(rows); i++) {
        char **tokens = rows[i];
        if (isTagName(tokens[0])) error("unexpected tag on line %d", i+1);
        if (! isStateName(tokens[0])) continue;
        int n = size(tokens);
        if (n < 4) error("incomplete rule on line %d", i+1);
        if (! isStateName(tokens[n-2])) {
            error("expecting target state on line %d", i+1);
        }
        if (findState(states, tokens[0]) < 0) {
            states = addState(states, tokens[0]);
        }
    }
    return states;
}

// Gather the information from the rules into the states.
void fillStates(char ***rows, state *states) {
    for (int i = 0; i < size(rows); i++) {
        char **tokens = rows[i];
        if (! isStateName(tokens[0])) continue;
        int n = size(tokens);
        int baseIndex = findState(states, tokens[0]);
        state *base = &states[baseIndex];
        int target = findState(states, tokens[n-2]);
        if (target < 0) error("state %s has no rules", tokens[n-2]);
        char *tag = tokens[n-1];
        for (int j = 1; j < n-2; j++) {
            char *s = tokens[j];
            if (strcmp(s, "\\") == 0) error("empty lookahead on line %d", i+1);
            if (s[0] == '\\' && 'a' <= s[1] && s[1] <= 'z' && s[2] == '\0') {
                if (s[1] != 's' && s[1] != 'n') {
                    error("bad lookahead on line %d", i+1);
                }
            }
            pattern pat;
            convert(s, target, tag, &pat);
            if (isRange(pat.s)) {
                base->ranges = resizeBy(base->ranges, 1);
                base->ranges[size(base->ranges) - 1] = pat;
            }
            else {
                int ch = pat.s[0];
                base->patterns[ch] = resizeBy(base->patterns[ch], 1);
                base->patterns[ch][size(base->patterns[ch]) - 1] = pat;
            }
        }
    }
}

// ---------- Ranges -----------------------------------------------------------
// A range such as 0..9 is equivalent to several one-character patterns, except
// that more specific patterns take precedence. Ranges are expanded by
// repeatedly finding a range with no subrange, and replacing it by
// one-character patterns for those characters not already handled.

bool subRange(char *s, char *t) {
    return s[0] >= t[0] && s[3] <= t[3];
}

bool overlap(char *s, char *t) {
    if (s[0] < t[0] && s[3] >= t[0] && s[3] < t[3]) return true;
    if (t[0] < s[0] && t[3] >= s[0] && t[3] < s[3]) return true;
    return false;
}

// Get an array of one-character strings.
char **getSingles() {
    char **singles = malloc(128 * sizeof(char *) + 256);
    char *bytes = (char *)(singles + 128);
    for (int ch = 0; ch < 128; ch++) {
        bytes[2*ch] = ch;
        bytes[2*ch+1] = '\0';
        singles[ch] = &bytes[2*ch];
    }
    return singles;
}

// Compare two ranges. Use reverse order of the left hand end character and
// forward order of the right hand end character, which ensures that a narrower
// subrange comes before a wider range. Return 0 for ranges which are the same,
// or which overlap.
int compareRanges(char *r, char *s) {
    char a = r[0], b = r[3], c = s[0], d = s[3];
    if (a > c) {
        if (b <= d) return -1;
        else if (b > c) return -1;
        else return 0;
    }
    else if (a == c) {
        if (b < d) return -1;
        else if (b == d) return 0;
        else return 1;
    }
    else {
        if (b >= d) return 1;
        else if (b < c) return -1;
        else return 0;
    }
}

// Sort a state's list of ranges. Report any ambiguous overlaps. 
void sortRanges(state *base) {
    pattern *list = base->ranges;
    int n = size(list);
    for (int i = 1; i < n; i++) {
        pattern r = list[i];
        int j = i - 1;
        while (j >= 0 && compareRanges(list[j].s, r.s) > 0) {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = r;
        if (j > 0 && compareRanges(list[j-1].s, r.s) == 0) {
            error("state %s has overlapping ranges", base->name);
        }
    }
}

// Expand a range into one-character patterns.
void derange(state *base, pattern *range, char **singles) {
    for (int ch = range->s[0]; ch <= range->s[3]; ch++) {
        pattern *list = base->patterns[ch];
        bool found = false;
        for (int i = 0; i < size(list); i++) {
            char *s = list[i].s;
            if (s[0] == ch && s[1] == '\0') { found = true; break; }
        }
        if (found) continue;
        int n = size(list);
        list = resizeBy(list, 1);
        list[n] = *range;
        list[n].s = singles[ch];
        base->patterns[ch] = list;
    }
}

// Expand all ranges in all states.
void derangeAll(state *states, char **singles) {
    for (int i = 0; i < size(states); i++) {
        state *base = &states[i];
        sortRanges(base);
        for (int j = 0; j < size(base->ranges); j++) {
            derange(base, &base->ranges[j], singles);
        }
    }
}

// ---------- Sorting ----------------------------------------------------------

// Sort the patterns in a list by decreasing length, so that when they are tried
// one after another in the scanner, the longest match is found first.
void sort(pattern *list) {
    int n = size(list);
    for (int i = 1; i < n; i++) {
        pattern p = list[i];
        int j = i - 1;
        while (j >= 0 && strlen(list[j].s) < strlen(p.s)) {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = p;
    }
}

// For each state, sort all the lists of patterns. 
void sortAll(state *states) {
    for (int i = 0; i < size(states); i++) {
        state *base = &states[i];
        for (int j = 0; j < 128; j++) {
            pattern *list = base->patterns[j];
            sort(list);
        }
    }
}

// ---------- Checks -----------------------------------------------------------
// Check that a scanner handles every input, generates only non-empty tokens,
// and never fails or gets stuck in an infinite loop.

// Scan the states to set their flags. Set the starter flag for a state which
// can occur at the start of a token, and the adder flag for a state which can
// occur after the start.
void scanPatterns(state *states) {
    states[0].starter = true;
    for (int s = 0; s < size(states); s++) {
        state *base = &states[s];
        for (int i = 0; i < 128; i++) {
            pattern *list = base->patterns[i];
            for (int j = 0; j < size(list); j++) {
                pattern *p = &list[j];
                int t = p->target;
                state *target = &states[t];
                if (p->tag != '+') target->starter = true;
                else if (! p->lookahead) target->adder = true;
            }
        }
    }
}

// For a state and a string appearing next in the input, follow lookahead
// patterns to determine whether an infinite loop is possible. (Ditto empty
// token.) TODO patterns in proper order.
void noLoop(state *states, int s, char *p) {
    // If visited, report loop. Mark visited.
    // Get list for first char and loop through it.
    // If it is a jump:
    //     adjust string to max of two
    //     noLoop(target, string)
    // Unmark visited.
}

// Check that no state has duplicate patterns.
void noDuplicates(state *states) {
    for (int i = 0; i < size(states); i++) {
        state *st = &states[i];
        for (int j = 0; j < 128; j++) {
            pattern *list = st->patterns[j];
            for (int k = 0; k < size(list); k++) {
                char *s = list[k].s;
                for (int m = k+1; m < size(list); m++) {
                    char *t = list[m].s;
                    if (strcmp(s,t) != 0) continue;
                    error("state %s has pattern %s twice", st->name, s);
                }
            }
        }
    }
}

// Deduce further starter and adder flags by following jumps. A jump is a
// lookahead pattern with no tag (so it passes control to another state with no
// progress). To allow for jump sequences, redo until there are no changes.
// A \s or \n jump can't occur at the start of a token, so don't transfer the
// starter flag in this case.


int main() {
    char *text = readFile("c.txt");
    printf("Chars: %lu\n", strlen(text));
    char **lines = splitLines(text);
    printf("Lines: %d\n", size(lines));
    char ***rows = makeRows(lines);
    printf("Rows: %d\n", size(rows));
    state *states = makeStates(rows);
    fillStates(rows, states);
    printf("States: %d\n", size(states));
    char **singles = getSingles();
    derangeAll(states, singles);
    sortAll(states);
    scanPatterns(states);
    noDuplicates(states);
    /*
    for (int i = 0; i < 127; i++) {
        if (i < 32 && i != '\n') continue;
        for (int j = 0; j < size(states[0].patterns[i]); j++) {
            printf("%d %s\n", j, states[0].patterns[i][j].s);
        }
    }
    */
    // checkStates(states);
    // pattern ***chart = makeChart(states);
    // byte *table = compile(chart);
    // printf("table %d %d\n", 256*size(states), size(table));

    // char in[100] = "   \n   b";
    // char out[100];
    // skip(in, out, 0);
    // out[7] = '\0';
    // decode(out);
    // printf("out: %s\n", out);

    free(text);
    release(lines);
    for (int i = 0; i < size(rows); i++) release(rows[i]);
    release(rows);
    for (int i = 0; i < size(states); i++) {
        for (int j = 0; j < 128; j++) release(states[i].patterns[j]);
        release(states[i].ranges);
        release(states[i].patterns);
    }
    release(states);
    free(singles);
}


// =============================================================================

/*
// A jump is a pair of states where there 
struct jump { state *base; char *text; state *target; };
typedef struct jump jump;


void deduce(jump *jumps) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < size(jumps); i++) {
            jump *p = &jumps[i];
            if (p->base->starter && ! p->target->starter) {
                p->target->starter = true;
                changed = true;
            }
            if (p->text[0] == ' ' || p->text[0] == '\n') continue;
            if (p->base->adder && ! p->target->adder) {
                p->target->adder = true;
                changed = true;
            }
        }
    }
}

// Report a state which could create an empty token because both the starter and
// ender tags are true.
void reportEmpty(state *states) {
    for (int i = 0; i < size(states); i++) {
        state *base = &states[i];
        if (! base->starter || ! base->ender) continue;
        error("state %s can create an empty token", base->name);
    }
}

// Check that a state handles a given character. 
void handles(state *st, char ch) {
    char s[] = { ch, '\0'};
    bool found = false;
    for (int i = 0; i < size(st->patterns); i++) {
        if (strcmp(st->patterns[i].s, s) == 0) found = true;
    }
    if (found) return;
    if (ch == ' ') error("state %s doesn't handle \\s", st->name);
    else if (ch == '\n') error("state %s doesn't handle \\n", st->name);
    else error("state %s doesn't handle %c", st->name, ch);
}

// Check that a state handles every character. If a state can only occur at the
// start of a token, it need not handle \s or \n. 
void complete(state *st) {
    for (char ch = '!'; ch <= '~'; ch++) handles(st, ch);
    if (st->adder) handles(st, ' ');
    if (st->adder) handles(st, '\n');
}

// Check every state, then deal with jumps.
void checkStates(state *states) {
    jump *jumps = allocate(sizeof(jump));
    for (int i = 0; i < size(states); i++) {
        state *st = &states[i];
        isDefined(st);
        noDuplicate(st);
        complete(st);
    }
    scanPatterns(states, jumps);
    deduce(jumps);
    printf("jumps %d\n", size(jumps));
    reportEmpty(states);
    for (int i=0; i<size(states); i++) {
        printf("%s", states[i].name);
        if (states[i].starter) printf(" starter");
        if (states[i].adder) printf(" adder");
        if (states[i].ender) printf(" ender");
        printf("\n");
    }
    release(jumps);
}


// TODO: check for infinite loops.
// Check that there can be no infinite loop of jumps on any particular input.
//void noloop(jump *jumps) {
// TODO: store jumps in states, maybe not separately. OR get rid of jumps.
// Mark jumps as visited rather than states.
// Check each as a starting point separately, using compatibility,
// and look for return to same state.
//}

// ---------- Compiling --------------------------------------------------------
// Compile the chart into a binary state transition table. The table has a row
// for each state, followed by an overflow area for lists which are more than
// one pattern long. Each row consists of 128 entries of two bytes each, one for
// each character. The scanner uses the current state and the next character to
// look up an entry. The entry may be an action for that single character, or an
// offset relative to the start of the table to a list of patterns starting with
// that character, with their actions.

typedef unsigned char byte;

// Fill in an action for a given pattern, as two bytes, one for the tag and one
// for the target state. The tag is compressed into 6 bits, similarly to
// base64 (i.e. an index into "A..Za..z0..9+"). That leaves room for a bit 0x40
// to indicate a lookahead action. For an action in the main table, the top
// bit 0x80 means the action is immediate.
void fillAction(byte *action, pattern *p, bool last) {
    int code, tag = p->tag;
    if ('A' <= tag && tag <= 'Z') code = tag - 'A';
    else if ('a' <= tag && tag <= 'z') code = 26 + (tag - 'a');
    else if ('0' <= tag && tag <= '9') code = 52 + (tag - '0');
    else code = 62;
    if (p->lookahead) code += 64;
    if (last) code += 128;
    action[0] = code;
    action[1] = p->target; 
}

char decode(char *s) {
    for (int i = 0; i < strlen(s); i++) {
        int n = s[i];
        s[i] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"[n];
    }
}

// When there is more than one pattern starting with the same character, enter
// the given offset into the table entry (in bigendian order).
void fillLink(byte *action, int offset) {
    action[0] = (offset >> 8) & 0xFF;
    action[1] = offset & 0xFF;
}

// Fill in a list of patterns as actions at the end of the table. Each action
// consists of two bytes as in fillAction, followed by a byte containing the
// number of characters in the pattern after the first, followed by those
// characters. (The list needs no termination because the last entry always
// matches.)
void fillList(pattern *ps, byte *table) {
    int np = size(ps);
    for (int i = 0; i < np; i++) {
        pattern *p = &ps[i];
        int len = strlen(p->s);
        byte *action = &table[size(table)];
        bool last = (i == np - 1);
        table = resizeBy(table, 2 + 1 + len);
        fillAction(action, p, last);
        action[2] = len;
        strncpy((char *)&action[4], p->s, len);
    }
}

byte *compile(pattern ***chart) {
    byte *table = allocate(sizeof(byte));
    table = resizeBy(table, 256 * size(chart));
    printf("table1: %d\n", size(table));
    for (int i = 0; i < size(chart); i++) {
        for (int j = 0; j < 128; j++) {
            byte *action = &table[256 * i + j];
            pattern *ps = chart[i][j];
            int n = size(ps);
            if (n == 0) {
                pattern p = { .tag='U', .target=0, .lookahead=false };
                fillAction(action, &p, true);
            }
            else if (n == 1) fillAction(action, &ps[0], true);
            else {
                int offset = size(table);
                fillLink(action, offset);
                fillList(ps, table);
            }
        }
    }
    return table;
}

// ---------- Scanning ---------------------------------------------------------
// A line of text is scanned using the binary state transition table, and an
// output line of text is produced.

// Compressed tag values representing '+' (add to token), 'G' and 'N'.
enum { ADD = 62, GAP = 6, NEWLINE = 13 };

// Skip spaces and newlines. Mark spaces with G and each newline with N.
int skip(char *in, char *out, int n) {
        printf("skip\n");
    int gap = n;
    while (in[n] == ' ' || in[n] == '\n') {
        printf("n: %d\n", n);
        if (in[n] == '\n') {
            out[n] = NEWLINE;
            if (n > gap) out[gap] = GAP;
            gap = n + 1;
        }
        else out[n] = ADD;
        n++;
    }
    if (n > gap) out[gap] = GAP;
    return n;
}

void scan(byte *table, char *in, char *out) {
    int state = 0, at = 0, token = 0;
    while (in[at] != '\n') {
        int ch = in[at];
        byte *action = &table[256*state + ch];
        bool immediate = (action[0] & 0x80) != 0;
        if (immediate) {
            bool lookahead = action[0] & 0x40;
            int tag = action[0] & 0x3F;
            if (! lookahead) {
                at++;
                if (ch == ' ') ;
            }
            state = action[1];
        }
    }
}

// ---------- Testing ----------------------------------------------------------
// A line in the language description starting with "> " is a test. A line below
// it starting with "< " shows expected results. The result has a one-character
// type under the first character of a token, with 'G' for a gap of spaces
// and 'N' for a newline.

// TODO:  testing.


// STATE PROPERTIES.
// Ender: rule has tag and lookahead including \s \n
// Can start a token: start + adder tag + after jump from starter.
// Jumps. Has lookahead not \s\n. No double jump (for same pattern).
*/

/*

// extract tags: (add GAP, NEWLINE, BADCH)
// extract states: starting states then continuing states

// An action has a target state and two token styles, one to terminate the
// current token before the pattern currently being matched, and one after. A
// style Skip means 'this action is not relevant in the current state'.
enum { Skip = CountStyles };
typedef unsigned short ushort;
struct action { style before, after; ushort target; };
typedef struct action action;

// A scanner has a table of actions indexed by state and pattern number. It has
// an indexes array of length 128 containing the pattern number of the patterns
// which start with a particular ASCII character. It has an offsets array of
// length 128 containing the corresponding offsets into the symbols. It has a
// symbols array containing the pattern strings, each preceded by its
// length in one byte. The symbols are sorted, longer first where one is a
// prefix of the other, and the symbols starting with each character are
// followed by an empty string to act as a default. For tracing, there are also
// arrays of original text, style names, state names, and patterns.
struct scanner {
    int nStates, nPatterns;
    action *table;
    ushort *indexes;
    ushort *offsets;
    char *symbols;
    char *text;
    char **styles;
    char **states;
    char **patterns;
};
typedef struct scanner scanner;

// ----------------------------------------------------------------------------
// Implement lists of pointers as arrays, preceded by the length.

// The maximum length of any list. Increase as necessary.
enum { MAX = 1000 };

void *newList() {
    void **xs = malloc((MAX+1) * sizeof(void *));
    xs[0] = (void *) 0;
    return xs + 1;
}

void freeList(void *xs) {
    void **ys = (void **) xs;
    free(ys - 1);
}

int length(void *xs) {
    void **ys = (void **) xs;
    return (intptr_t) ys[-1];
}

void setLength(void *xs, int n) {
    assert(n <= MAX);
    void **ys = (void **) xs;
    ys[-1] = (void *) (intptr_t) n;
}

void add(void *xs, void *x) {
    void **ys = (void **) xs;
    ys[length(ys)] = x;
    setLength(ys, length(ys) + 1);
}

// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Build a scanner from the data gathered so far. Build an array of style names,
// create a blank action table, and fill each row from a rule.

// Create the list of style names. Add "More" corresponding to CountStyles
char **makeStyles() {
    char **styles = newList();
    for (int i = 0; i < CountStyles; i++) {
        char *name = styleName(i);
        add(styles, name);
    }
    add(styles, "More");
    return styles;
}

// Create a blank action table. It is a one-dimensional array of action
// structures. It can be treated as a two-dimensional array, indexed by state
// and pattern, by casting it to type action(*)[nPatterns]. The actions are
// initialized to Skip.
action *makeTable(int nStates, int nPatterns) {
    action *rawTable = malloc(nStates * nPatterns * sizeof(action *));
    action (*table)[nPatterns] = (action(*)[nPatterns]) rawTable;
    for (int s = 0; s < nStates; s++) {
        for (int p = 0; p < nPatterns; p++) {
            table[s][p] = (action) { .before=Skip, .after=Skip, .target=0 };
        }
    }
    return rawTable;
}

// Enter a rule into the table. A rule with no patterns is a default, matching
// the empty string. The default is entered in all the empty string positions,
// so that each run of patterns starting with the same character ends with the
// default, to avoid special case default handling when the scanner executes.
void addRule(scanner *sc, char **tokens) {
    action (*table)[sc->nPatterns] = (action(*)[sc->nPatterns]) sc->table;
    int n = length(tokens);
    int state = search(sc->states, tokens[0]);
    int before = search(sc->styles, tokens[1]);
    int after = search(sc->styles, tokens[n-2]);
    int target = search(sc->states, tokens[n-1]);
    assert(state >= 0 && before >= 0 && after >= 0 && target >= 0);
    if (n == 4) {
        for (int pattern = 0; pattern < sc->nPatterns; pattern++) {
            if (sc->patterns[pattern][0] == '\0') {
                action *p = &table[state][pattern];
                if (p->before != Skip) continue;
                *p = (action) { .before=before, .after=after, .target=target };
            }
        }
    }
    else for (int i = 2; i < n - 2; i++) {
        int pattern = search(sc->patterns, tokens[i]);
        assert(pattern >= 0);
        action *p = &table[state][pattern];
        if (p->before != Skip) continue;
        *p = (action) { .before = before, .after = after, .target = target };
    }
}

scanner *newScanner(char const *path) {
    scanner *sc = malloc(sizeof(scanner));
    sc->text = readFile(path);
    char ***rules = readRules(sc->text);
    sc->states = findStates(rules);
    sc->patterns = findPatterns(rules);
    sc->styles = makeStyles();
    sort(sc->patterns);
    extend(sc->patterns);
    sc->nStates = length(sc->states);
    sc->nPatterns = length(sc->patterns);
    sc->indexes = findIndexes(sc->patterns);
    sc->symbols = compress(sc->patterns);
    sc->offsets = findOffsets(sc->symbols, sc->indexes);
    sc->table = makeTable(sc->nStates, sc->nPatterns);
    for (int i = 0; i < length(rules); i++) addRule(sc, rules[i]);
    for (int i = 0; i < length(rules); i++) freeList(rules[i]);
    freeList(rules);
    return sc;
}

void freeScanner(scanner *sc) {
    free(sc->table);
    free(sc->indexes);
    free(sc->offsets);
    free(sc->symbols);
    free(sc->text);
    freeList(sc->styles);
    freeList(sc->states);
    freeList(sc->patterns);
    free(sc);
}

// Scan a line of source text to produce a line of style bytes. At each step:
// 1) find the patterns starting with the next char.
// 2) try to match them with the text, where relevant.
// 3) move past the matched pattern.
// 4) continue or end the token and move to the next state.
void scan(scanner *sc, int row, char *line, char *styles) {
    int state = 0;
    int s = 0, i = 0, n = length(line) - 1;
    while (i < n || s < i) {
        int ch = line[i];
        if ((ch & 0x80) != 0) ch = 'A';
        int start = sc->offsets[ch];
    }
    
        int matched = empty;
        int old = i;
        for (int p = start; p < end; p++) {
            action act = sc->table[state][p];
            if (act.style == Skip) continue;
            char *pattern = sc->patterns[p];
            int len = strlen(pattern);
            if (! match(line, i, len, pattern)) continue;
            if (TRACE) printf("scan %s %s", sc->states[state], pattern);
            i = i + len;
            matched = p;
            break;
        }
        if (TRACE && matched == empty) printf("scan %s", sc->states[state]);
        action act = sc->table[state][matched];
        ushort st = act.style;
        ushort base = st & NOFLAGS;
        state = act.target;
        if (TRACE) {
            printf(" %s", sc->states[state]);
            if (st == More) printf("\n");
            else if ((st & BEFORE) != 0) printf("    <%s\n", styleName(base));
            else printf("    >%s\n", styleName(base));
        }
        if (st == More || s == i) continue;
        C(styles)[s++] = addStyleFlag(base, START);
        if ((st & BEFORE) != 0) { while (s < old) C(styles)[s++] = base; }
        else while (s < i) C(styles)[s++] = base;
    }
    C(styles)[s] = addStyleFlag(GAP, START);
    if (TRACE) printf("end state %s\n", sc->states[state]);
    sc->endStates = setEndState(sc->endStates, row, state);
    
}

// ----------------------------------------------------------------------------
// Tests.

// Test that readTokens divides a line at the spaces into tokens.
void testReadTokens() {
    char text[] = "";
    char **tokens = readTokens(text);
    assert(length(tokens) == 0);
    freeList(tokens);
    char text1[] = "a bb ccc dddd";
    tokens = readTokens(text1);
    assert(length(tokens) == 4);
    assert(strcmp(tokens[0], "a") == 0);
    assert(strcmp(tokens[1], "bb") == 0);
    assert(strcmp(tokens[2], "ccc") == 0);
    assert(strcmp(tokens[3], "dddd") == 0);
    freeList(tokens);
    char text2[] = " a   bb  ccc      dddd   ";
    tokens = readTokens(text2);
    assert(length(tokens) == 4);
    assert(strcmp(tokens[0], "a") == 0);
    assert(strcmp(tokens[1], "bb") == 0);
    assert(strcmp(tokens[2], "ccc") == 0);
    assert(strcmp(tokens[3], "dddd") == 0);
    freeList(tokens);
}

// Check that a list of strings matches a space separated description. An empty
// string in the list is represented as a question mark in the description. This
// uses readTokens to split up the description.
bool check(char **list, char *s) {
    char s2[strlen(s)+1];
    strcpy(s2, s);
    char **strings = readTokens(s2);
    bool ok = true;
    if (length(list) != length(strings)) ok = false;
    else for (int i = 0; i < length(list); i++) {
        if (list[i][0] == '\0' && strings[i][0] == '?') { }
        else if (strcmp(list[i], strings[i]) != 0) ok = false;
    }
    freeList(strings);
    return ok;
}

// Test that readLines divides lines, coping with different line endings.
void testReadLines() {
    char text[] = "abc\ndef\n";
    char **lines = readLines(text);
    assert(check(lines, "abc def"));
    freeList(lines);
    char text2[] = "abc\r\ndef\r\n";
    lines = readLines(text2);
    assert(check(lines, "abc def"));
    freeList(lines);
}

// Test that the two special cases are handled.
void testNormalize() {
    char line[] = "s _ t";
    char **tokens = readTokens(line);
    normalize(tokens);
    assert(strcmp(tokens[0], "s") == 0);
    assert(strcmp(tokens[1], " ") == 0);
    assert(strcmp(tokens[2], "t") == 0);
    freeList(tokens);
    char line2[] = "s t";
    tokens = readTokens(line2);
    normalize(tokens);
    assert(strcmp(tokens[0], "s") == 0);
    assert(strcmp(tokens[1], "") == 0);
    assert(strcmp(tokens[2], "t") == 0);
    freeList(tokens);
}

// Test that ranges are extended to one-character strings.
void testextendRanges() {
    char line[] = "s a..c t";
    char **tokens = readTokens(line);
    extendRanges(tokens);
    assert(check(tokens, "s a b c t"));
    freeList(tokens);
}

// Test that the before and after styles for a rule are extracted, with More as
// the default.
void testextendStyles() {
    char line[] = "s p t";
    char **tokens = readTokens(line);
    extendStyles(tokens);
    assert(check(tokens, "s More p More t"));
    freeList(tokens);
    char line2[] = "s:X p t";
    tokens = readTokens(line2);
    extendStyles(tokens);
    assert(check(tokens, "s X p More t"));
    freeList(tokens);
    char line3[] = "s p X:t";
    tokens = readTokens(line3);
    extendStyles(tokens);
    assert(check(tokens, "s More p X t"));
    freeList(tokens);
}

// Test sorting, with the prefix rule.
void testSort() {
    char line[] = "s a b aa c ba ccc sx";
    char **patterns = readTokens(line);
    sort(patterns);
    assert(check(patterns, "aa a ba b ccc c sx s"));
    freeList(patterns);
}

// Test that runs of patterns starting with the same character are divided by
// empty strings, and that findIndexes identifies the runs.
void testIndexes() {
    char line[] = "b bc d de";
    char **patterns = readTokens(line);
    extend(patterns);
    assert(check(patterns, "? b bc ? d de ?"));
    ushort *indexes = findIndexes(patterns);
    assert(indexes[0] == 0);
    assert(indexes['a'] == 0);
    assert(indexes['b'] == 1);
    assert(indexes['c'] == 3);
    assert(indexes['d'] == 4);
    assert(indexes['e'] == 6);
    assert(indexes[127] == 6);
    freeList(patterns);
    free(indexes);
}

// Test that patterns are compressed into a symbols array, with leading length
// bytes, and an updated patterns array.
void testCompress() {
    char line[] = "b bc d de";
    char **patterns = readTokens(line);
    extend(patterns);
    assert(check(patterns, "? b bc ? d de ?"));
    char *symbols = compress(patterns);
    assert(memcmp(symbols, "\0\0\1b\0\2bc\0\0\0\1d\0\2de\0\0\0", 20) == 0);
    assert(patterns[0] == &symbols[1]);
    assert(patterns[1] == &symbols[3]);
    assert(patterns[2] == &symbols[6]);
    assert(patterns[3] == &symbols[10]);
    assert(patterns[4] == &symbols[12]);
    assert(patterns[5] == &symbols[15]);
    assert(patterns[6] == &symbols[19]);
    freeList(patterns);
    free(symbols);
}

// Test that findOffsets converts indexes into offsets in the symbols array.
void testOffsets() {
    char line[] = "b bc d de";
    char **patterns = readTokens(line);
    extend(patterns);
    char *symbols = compress(patterns);
    assert(memcmp(symbols, "\0\0\1b\0\2bc\0\0\0\1d\0\2de\0\0\0", 20) == 0);
    ushort *indexes = findIndexes(patterns);
    ushort *offsets = findOffsets(symbols, indexes);
    assert(offsets[0] == 0);
    assert(offsets['a'] == 0);
    assert(offsets['b'] == 2);
    assert(offsets['c'] == 9);
    assert(offsets['d'] == 11);
    assert(offsets['e'] == 18);
    assert(offsets[127] == 18);
    freeList(patterns);
    free(indexes);
    free(symbols);
    free(offsets);
}

int main() {
    setbuf(stdout, NULL);
    testReadTokens();
    testReadLines();
    testNormalize();
    testextendRanges();
    testextendStyles();
    testSort();
    testIndexes();
    testCompress();
    testOffsets();
    scanner *sc = newScanner("c.txt");
    freeScanner(sc);
    printf("Tests pass\n");
}
*/
/*
// -----------------------------------------------------------------------------
// Enter a rule into the table.
static void addRule(strings *line, action **table, strings *states,
    strings *patterns, strings *styles)
{
    int n = length(line);
    ushort row = search(states, S(line)[0]);
    ushort target = search(states, S(line)[n-2]);
    char *act = S(line)[n-1];
    assert(row >= 0 && target >= 0);
    ushort style = 0;
    if (strlen(act) > 0) {
        char *styleName = &act[1];
        style = search(styles, styleName);
        if (style < 0) printf("Unknown style %s\n", styleName);
    }
    if (act[0] == '\0') style = More;
    else if (act[0] == '<') style |= BEFORE;
    else if (act[0] != '>') printf("Unknown action %s\n", act);
    action *actions = table[row];
    if (n == 3) {
        int col = search(patterns, "");
        assert(col >= 0);
        if (actions[col].style == Skip) {
            actions[col].style = style;
            actions[col].target = target;
        }
    }
    else for (int i = 1; i < n - 2; i++) {
        char *s = S(line)[i];
        int col = search(patterns, s);
        assert(col >= 0);
        if (actions[col].style != Skip) continue;
        actions[col].style = style;
        actions[col].target = target;
    }
}

// Create a blank table.
static action **makeTable(strings *states, strings *patterns)
{
    int height = length(states);
    int width = length(patterns);
    action **table = malloc(height * sizeof(action *));
    for (int r = 0; r < height; r++) {
        table[r] = malloc(width * sizeof(action));
        for (int c = 0; c < width; c++) table[r][c].style = Skip;
    }
    return table;
}

// Fill a table from the rules.
static void fillTable(action **table, strings **rules, strings *states,
    strings *patterns, strings *styles)
{
    for (int i = 0; rules[i] != NULL; i++) {
        strings *rule = rules[i];
        addRule(rule, table, states, patterns, styles);
    }
}

void changeLanguage(scanner *sc, char *lang) {
    char *content = readLanguage(lang);
    strings *states = newStrings();
    strings *patterns = newStrings();
    find(patterns, "");
    strings *lines = splitLines(content);
    strings **rules = splitTokens(lines);
    findNames(rules, states, patterns);
    freeData(sc);
    sc->chars = content;
    sort(patterns);
    strings *styles = makeStyles();
    action **table = makeTable(states, patterns);
    fillTable(table, rules, states, patterns, styles);
    sc->table = table;
    sc->height = length(states);
    sc->width = length(patterns);
    sc->offsets = makeOffsets(patterns);
    sc->states = S(states);
    sc->states = realloc(sc->states, sc->height * sizeof(char *));
    free(states);
    sc->patterns = S(patterns);
    sc->patterns = realloc(sc->patterns, sc->width * sizeof(char *));
    free(patterns);
    for (int i = 0; rules[i] != NULL; i++) freeList(rules[i]);
    free(rules);
    freeList(lines);
    freeList(styles);
}

static bool TRACE;

// Match part of a list against a string.
static inline bool match(chars *line, int i, int n, char *pattern) {
    return strncmp(C(line) + i, pattern, n) == 0;
}

// Scan a line of source text to produce a line of style bytes. At each step:
// 1) find the range, start <= x < end, of patterns starting with the next char.
// 2) try to match the ones that are relevant.
// 3) move past the matched pattern.
// 4) if no relevant patterns match, use the empty pattern as a default.
// 5) continue or end the token and move to the next state.
void scan(scanner *sc, int row, chars *line, chars *styles) {
    resize(styles, length(line));
    int state = (row == 0) ? 0 : sc->endStates[row - 1];
    int s = 0, i = 0, n = length(line) - 1;
    int empty = sc->offsets[127];
    while (i < n || s < i) {
        int ch = C(line)[i];
        if ((ch & 0x80) != 0) ch = 'A';
        int start = sc->offsets[ch];
        int end = sc->offsets[ch + 1];
        int matched = empty;
        int old = i;
        for (int p = start; p < end; p++) {
            action act = sc->table[state][p];
            if (act.style == Skip) continue;
            char *pattern = sc->patterns[p];
            int len = strlen(pattern);
            if (! match(line, i, len, pattern)) continue;
            if (TRACE) printf("scan %s %s", sc->states[state], pattern);
            i = i + len;
            matched = p;
            break;
        }
        if (TRACE && matched == empty) printf("scan %s", sc->states[state]);
        action act = sc->table[state][matched];
        ushort st = act.style;
        ushort base = st & NOFLAGS;
        state = act.target;
        if (TRACE) {
            printf(" %s", sc->states[state]);
            if (st == More) printf("\n");
            else if ((st & BEFORE) != 0) printf("    <%s\n", styleName(base));
            else printf("    >%s\n", styleName(base));
        }
        if (st == More || s == i) continue;
        C(styles)[s++] = addStyleFlag(base, START);
        if ((st & BEFORE) != 0) { while (s < old) C(styles)[s++] = base; }
        else while (s < i) C(styles)[s++] = base;
    }
    C(styles)[s] = addStyleFlag(GAP, START);
    if (TRACE) printf("end state %s\n", sc->states[state]);
    sc->endStates = setEndState(sc->endStates, row, state);
}

// Check a list of tokens produced by readLine.
static bool checkReadLine(char *t, int n, char *e[n]) {
    char tc[100];
    strcpy(tc, t);
    strings *ts = newStrings();
    readLine(tc, ts);
    if (length(ts) != n) return false;
    for (int i = 0; i < n; i++) {
        char *t = S(ts)[i];
        if (strcmp(t, e[i]) != 0) return false;
    }
    freeList(ts);
    return true;
}

// Check that readLine generates a suitable list of tokens (state, before-style,
// patterns, after-style, state).
static void testReadLine() {
    assert(checkReadLine("s s", 3, (char *[]) {"s","s",""}));
    assert(checkReadLine("s _ s", 4, (char *[]) {"s"," ","s",""}));
    assert(checkReadLine("s A..B s", 5, (char *[]) {"s","A","B","s",""}));
    assert(checkReadLine("s _..! s", 5, (char *[]) {"s"," ","!","s",""}));
    assert(checkReadLine("s ! s >OP", 4, (char *[]) {"s","!","s",">OP"}));
    assert(checkReadLine("s ! s <OP", 4, (char *[]) {"s","!","s","<OP"}));
    assert(checkReadLine("s s >OP", 3, (char *[]) {"s","s",">OP"}));
}

static bool check(scanner *sc, int row, char *l, char *expect) {
    chars *line = newChars();
    resize(line, strlen(l));
    strncpy(C(line), l, strlen(l));
    chars *styles = newChars();
    scan(sc, row, line, styles);
    for (int i = 0; i < length(line); i++) {
        char st = C(styles)[i];
        char letter = styleName(clearStyleFlags(st))[0];
        if (! hasStyleFlag(st, START)) letter = tolower(letter);
        C(styles)[i] = letter;
    }
    bool matched = match(styles, 0, length(line), expect);
    freeList(line);
    freeList(styles);
    return matched;
}

static void test() {
    scanner *sc = newScanner();
    assert(check(sc, 0, "x\n", "WG"));
    assert(check(sc, 0, "x y\n", "WGWG"));
    assert(check(sc, 0, "xy, z\n", "WwSGWG"));
    changeLanguage(sc, "c");
    assert(check(sc, 0, "x\n", "IG"));
    assert(check(sc, 0, "x y\n", "IGIG"));
    assert(check(sc, 0, "int x;\n", "KkkGISG"));
    assert(check(sc, 0, "intx;\n", "IiiiSG"));
    assert(check(sc, 0, "//xy\n", "NnnnG"));
    assert(check(sc, 0, "/" "* one\n", "CcGCccG"));
    assert(check(sc, 1, "x*" "/y\n", "CccIG"));
    freeScanner(sc);
}

// Trace scanning of a problem example.
static void trace() {
    scanner *sc = newScanner();
    changeLanguage(sc, "c");
    assert(check(sc, 0, "//x\n", "NnnG"));
    assert(check(sc, 0, "//x \n", "NnnGG"));
    freeScanner(sc);
}

int main(int n, char *args[n]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    testReadLine();
    TRACE = false;
    if (TRACE) trace();
    else test();
    printf("Scan module OK\n");
    return 0;
}
*/