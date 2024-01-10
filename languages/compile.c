// Snipe editor. Free and open source, see licence.txt.

// Compile a language definition. Read in a file such as c.txt, check the rules
// for consistency, run the tests and, if everything succeeds, write out a
// compact state table in binary file c.bin.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Use the kind constants from the main editor.
// Kinds are called types in the languages documentation page.
#include "../src/kinds.h"

// Check equality of two strings.
bool equal(char *s, char *t) {
    return strcmp(s, t) == 0;
}

// Check if one string is a prefix of another.
bool prefix(char *s, char *t) {
    if (strlen(s) >= strlen(t)) return false;
    if (strncmp(s, t, strlen(s)) == 0) return true;
    return false;
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

// ---------- Objects and Lists ------------------------------------------------
// All memory is allocated through newObject() and newList(), and each
// allocation is remembered in the store, which is a list of all allocations.
// An object is a string or structure which is allocated separately and does
// not move. A list is a dynamic array which has a prefixed header containing
// the length, capacity, unit size, and an index in the store to handle
// reallocations. The call adjust(a,d) changes the length. It must be called
// with a = adjust(a,d) in case a is reallocated. If any function f(a) calls
// adjust(a,d), it must be called with a = f(a) in case a gets reallocated.

struct header { int length, max, unit, index; void *align[]; };
typedef struct header header;

// A global list of allocations, for easy freeing. (Ironically, global links to
// all allocations prevent them from being reported as memory leaks anyway!)
header **store = NULL;

// Create the store as a list of pointers.
void newStore() {
    int max0 = 1;
    int ptr = sizeof(void *);
    header *h = malloc(sizeof(header) + max0 * ptr);
    *h = (header) { .length = 0, .max = max0, .unit = ptr, .index = -1 };
    store = (header **)(h + 1);
}

int length(void *a) {
    header *h = (header *) a - 1;
    return h->length;
}

int max(void *a) {
    header *h = (header *) a - 1;
    return h->max;
}

// Adjust list length from n to n+d. Return the possibly reallocated array.
void *adjust(void *a, int d) {
    header *h = (header *) a - 1;
    h->length += d;
    if (h->length <= h->max) return a;
    while (h->length > h->max) h->max = 2 * h->max;
    h = realloc(h, sizeof(header) + h->max * h->unit);
    if (h->index >= 0) store[h->index] = h;
    return h + 1;
}

// Set the list length to 0.
void clear(void *a) {
    header *h = (header *) a - 1;
    h->length = 0;
}

// Allocate a string or structure directly, and put it in the store.
void *newObject(int bytes) {
    if (store == NULL) newStore();
    void *p = malloc(bytes);
    int index = length(store);
    store = adjust(store, +1);
    store[index] = p;
    return p;
}

void *newList(int unit) {
    if (store == NULL) newStore();
    int max0 = 1;
    int index = length(store);
    store = adjust(store, +1);
    header *h = malloc(sizeof(header) + max0 * unit);
    *h = (header) { .length = 0, .max = max0, .unit = unit, .index = index };
    store[index] = h;
    return h + 1;
}

void freeAll() {
    for (int i = 0; i < length(store); i++) free(store[i]);
    header *h = (header *) store - 1;
    free(h);
}

// ---------- Lines ------------------------------------------------------------
// Read in a language description as a character array, normalize, and split the
// text into lines, in place.

// Read file as string, ignore I/O errors, add final newline if necessary.
char *readFile(char *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *text = newList(1);
    text = adjust(text, size+2);
    fread(text, 1, size, file);
    if (text[size-1] != '\n') text[size++] = '\n';
    text[size] = '\0';
    fclose(file);
    return text;
}

// Deal with \r\n and \r line endings, report bad characters.
void normalize(char *text) {
    int n = 1;
    for (int i = 0; text[i] != '\0'; i++) {
        if ((text[i] & 0x80) != 0) error("non-ascii character on line %d", n);
        if (text[i] == '\r' && text[i+1] == '\n') text[i] = ' ';
        else if (text[i] == '\r') text[i] = '\n';
        if (text[i] == '\n') n++;
        else if (text[i] < ' ') error("control character on line %d", n);
        else if (text[i] > '~') error("control character on line %d", n);
    }
}

// Remove leading and trailing spaces from a line.
char *trim(char *line) {
    while (line[0] == ' ') line++;
    int n = strlen(line);
    while (n > 0 && line[n-1] == ' ') n--;
    line[n] = '\0';
    return line;
}

// Split a text into trimmed lines.
char **splitLines(char *text) {
    int start = 0;
    char **lines = newList(sizeof(char *));
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i] = '\0';
        char *line = trim(&text[start]);
        int n = length(lines);
        lines = adjust(lines, +1);
        lines[n] = line;
        start = i + 1;
    }
    return lines;
}

// Stage 1: read file, split into lines.
char **getLines(char *path) {
    char *text = readFile(path);
    normalize(text);
    char **lines = splitLines(text);
    return lines;
}

// ---------- Rules ------------------------------------------------------------
// Find the lines containing rules, and split each into a list of strings.

// A rule is a line number and a list of strings.
struct rule { int line; char **strings; };
typedef struct rule Rule;

// Split a line into strings in place.
char **splitStrings(char *line) {
    char **strings = newList(sizeof(char *));
    int start = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] != ' ') continue;
        line[i] = '\0';
        if (i > 0 && line[i-1] != '\0') {
            int n = length(strings);
            strings = adjust(strings, +1);
            strings[n] = &line[start];
        }
        start = i + 1;
    }
    int n = length(strings);
    strings = adjust(strings, +1);
    strings[n] = &line[start];
    return strings;
}

// Stage 2: extract the rules.
Rule **getRules(char **lines) {
    Rule **rules = newList(sizeof(Rule *));
    for (int i = 0; i < length(lines); i++) {
        if (! islower(lines[i][0])) continue;
        int n = length(rules);
        rules = adjust(rules, +1);
        rules[n] = newObject(sizeof(Rule));
        rules[n]->line = i+1;
        rules[n]->strings = splitStrings(lines[i]);
    }
    return rules;
}

// ---------- States -----------------------------------------------------------
// A state has a row number (its row in the final table), a name, and an array
// of patterns. It has flags to say whether it occurs at or after the start of
// tokens. It has a visited flag used when checking for infinite loops. It has
// a partner state if it begins with both start and after flags set, and so
// gets split into a pair.

// Forwards reference to patterns.
typedef struct pattern Pattern;

typedef struct state State;
struct state {
    int row; char *name; Pattern **patterns;
    bool start, after, visited;
    State *partner;
};

// Find an existing state by name, returning its index or -1.
int findState(State **states, char *name) {
    for (int i = 0; i < length(states); i++) {
        if (equal(states[i]->name, name)) return i;
    }
    return -1;
}

// Add a new empty state with the given name. Give it a row number.
State **addState(State **states, char *name) {
    State *state = newObject(sizeof(State));
    Pattern **ps = newList(sizeof(Pattern *));
    int n = length(states);
    *state = (State) {
        .row = n, .name = name, .patterns = ps,
        .start = false, .after = false, .visited = false,
        .partner = NULL
    };
    states = adjust(states, +1);
    states[n] = state;
    return states;
}

// Stage 3: get the states from the rules. Optionally print.
State **getStates(Rule **rules, bool print) {
    State **states = newList(sizeof(State *));
    for (int i = 0; i < length(rules); i++) {
        Rule *rule = rules[i];
        char *base = rule->strings[0];
        int row = findState(states, base);
        if (row < 0) states = addState(states, base);
    }
    if (print) for (int i = 0; i < length(states); i++) {
        printf("%d: %s\n", states[i]->row, states[i]->name);
    }
    return states;
}

// ---------- Patterns ---------------------------------------------------------
// A pattern object holds a single pattern from the source text, with supporting
// information. It has an unescaped pattern string, a lookahead flag, a soft
// flag, and the line number, base and target states and type from its rule.
// A pattern object can be regarded as a one-pattern mini-rule.

struct pattern {
    char *string; bool look, soft; int line; State *base, *target; int type;
};
typedef struct pattern Pattern;

// Deal with the escape conventions in the most recently added pattern.
//   \s     -> space
//   \n     -> newline
//   \\     -> backslash
//   \|     -> vertical bar
//   |      -> \n..~ plus lookahead flag
//   |...   -> ... plus lookahead flag
Pattern **unescape(Pattern **patterns, int line) {
    int n = length(patterns);
    Pattern *p = patterns[n-1];
    char *s = p->string;
    int sn = strlen(s);
    if (s[0] == '|') p->look = true;
    for (int i = 0, j = 0; i <= sn; i++) {
        if (s[i] == '|') {
            if (i > 0) error("bad pattern on line %d", line);
        }
        else if (s[i] == '\\') {
            char ch = s[++i];
            if (ch == 's') s[j++] = ' ';
            else if (ch == 'n') s[j++] = '\n';
            else if (ch == '\\') s[j++] = '\\';
            else if (ch == '|') s[j++] = '|';
            else error("bad escape \\%c on line %d", ch, line);
        }
        else s[j++] = s[i];
    }
    if (strlen(s) == 0) p->string = "\n..~";
    return patterns;
}

// Collect patterns from a rule in the source text.
Pattern **collectPatterns(Pattern **patterns, Rule *rule, State **states) {
    int line = rule->line;
    char **strings = rule->strings;
    int n = length(strings);
    if (n < 3) error("incomplete rule on line %d", line);
    State *base = states[findState(states, strings[0])];
    char *last = strings[n-1];
    int type = More;
    if (isupper(last[0])) {
        type = findKind(last);
        if (type < 0) error("unknown type %s on line %d", last, line);
        n--;
        if (n < 2) error("incomplete rule on line %d", line);
    }
    if (! islower(strings[n-1][0])) error("expecting target on line %d", line);
    int t = findState(states, strings[n-1]);
    if (t < 0) error ("undefined target state on line %d", line);
    State *target = states[t];
    for (int i = 1; i < n-1; i++) {
        char *string = strings[i];
        int r = length(patterns);
        patterns = adjust(patterns, +1);
        patterns[r] = newObject(sizeof(Pattern));
        *patterns[r] = (Pattern) {
            .line = line, .base = base, .target = target, .look = false,
            .soft = false, .string = string, .type = type
        };
        patterns = unescape(patterns, line);
    }
    return patterns;
}

// Print a pattern string by putting back the escapes. Return the number of
// characters printed.
int printPattern(Pattern *p) {
    char *s = p->string;
    int sn = strlen(s);
    int j = 0;
    if (p->look) { printf("|"); j++; }
    for (int i = 0; i < sn; i++) {
        if (s[i] == ' ') { printf("\\s"); j += 2; }
        else if (s[i] == '\n') { printf("\\n"); j += 2; }
        else if (s[i] == '\\') { printf("\\\\"); j += 2; }
        else if (s[i] == '|') { printf("\\|"); j += 2; }
        else { printf("%c", s[i]); j++; }
    }
    return j;
}

// Display a pattern as a mini-rule.
void printPatternRule(Pattern *p) {
    printf("%-10s ", p->base->name);
    int j = printPattern(p);
    while (j < 15) { printf(" "); j++; }
    printf("%-10s ", p->target->name);
    if (p->type != More) printf("%-10s", kindName(p->type));
    if (p->soft) printf("(soft)");
    printf("\n");
}

// Check whether patterns p and q can be treated as successive range elements.
bool compatible(Pattern *p, Pattern *q) {
    if (p->look != q->look) return false;
    if (strlen(p->string) != 1 || strlen(q->string) != 1) return false;
    if (p->string[0] == ' ' || p->string[0] == '\n') return false;
    if (q->string[0] == ' ' || q->string[0] == '\n') return false;
    if (p->string[0] + 1 != q->string[0]) return false;
    if (p->target != q->target) return false;
    if (p->type != q->type) return false;
    return true;
}

// Print a state's patterns as mini-rules. After range expansion and sorting, a
// state may have runs of many one-character patterns. Gather a run and print
// it as a range.
void printState(State *state) {
    Pattern **ps = state->patterns;
    for (int i = 0; i < length(ps); i++) {
        int j = i;
        while (j < length(ps)-1 && compatible(ps[j], ps[j+1])) j++;
        if (j == i) printPatternRule(ps[i]);
        else {
            Pattern range = *ps[i];
            char string[7];
            sprintf(string, "%c..%c", ps[i]->string[0], ps[j]->string[0]);
            range.string = string;
            printPatternRule(&range);
            i = j;
        }
    }
    printf("\n");
}

// Stage 4: collect the patterns from the rules. Optionally print the states.
void getPatterns(Rule **rules, State **states, bool print) {
    for (int i = 0; i < length(rules); i++) {
        Rule *rule = rules[i];
        State *base = states[findState(states, rule->strings[0])];
        base->patterns = collectPatterns(base->patterns, rules[i], states);
    }
    if (print) for (int i=0; i < length(states); i++) printState(states[i]);
}

// ---------- Ranges -----------------------------------------------------------
// Expand ranges such as 0..9 or |0..9 to multiple one-character patterns, with
// more specific patterns (subranges and individual characters) taking
// precedence. Then sort the patterns in each state. Then add a soft flag for a
// close bracket pattern if it is followed by another close bracket pattern for
// the same pattern string. Also add a soft flag for a lookahead pattern where
// there is a non-lookahead pattern for the same string.

// Single-character strings covering \n \s !..~ for expanding ranges.
char singles[128][2];

// Fill in the singles array.
void fill() {
    for (int ch = 0; ch < 128; ch++) {
        singles[ch][0] = ch;
        singles[ch][1] = '\0';
    }
}

// Check for a range or subranges or overlapping ranges.
bool isRange(char *s) {
    return (strlen(s) == 4 && s[1] == '.' && s[2] == '.');
}

bool subRange(char *s, char *t) {
    return s[0] >= t[0] && s[3] <= t[3];
}

bool overlap(char *s, char *t) {
    if (s[0] < t[0] && s[3] >= t[0] && s[3] < t[3]) return true;
    if (t[0] < s[0] && t[3] >= s[0] && t[3] < s[3]) return true;
    return false;
}

// Add a singleton pattern from a range, if not already handled.
Pattern **addSingle(Pattern **patterns, Pattern *range, int ch) {
    if (singles[1][0] == 0) fill();
    int n = length(patterns);
    for (int i = 0; i < n; i++) {
        char *s = patterns[i]->string;
        if (s[0] == ch && s[1] == '\0') return patterns;
    }
    patterns = adjust(patterns, +1);
    patterns[n] = newObject(sizeof(Pattern));
    *patterns[n] = *range;
    patterns[n]->string = singles[ch];
    return patterns;
}

// Remove the i'th pattern from a list. The patterns are not yet sorted, so just
// replace it with the last pattern.
void delete(Pattern **patterns, int i) {
    int n = length(patterns);
    patterns[i] = patterns[n-1];
    adjust(patterns, -1);
}

// Expand a range into singles, and add them if not already handled.
Pattern **derange(Pattern **patterns, Pattern *range) {
    char *s = range->string;
    for (int ch = s[0]; ch <= s[3]; ch++) {
        if (ch > '\n' && ch < ' ') continue;
        patterns = addSingle(patterns, range, ch);
    }
    return patterns;
}

// Repeatedly find a most specific range, and expand it.
Pattern **derangeList(Pattern **patterns) {
    int index = 1;
    while (index >= 0) {
        index = -1;
        for (int i = 0; i < length(patterns); i++) {
            char *s = patterns[i]->string;
            if (! isRange(s)) continue;
            if (index >= 0) {
                char *t = patterns[index]->string;
                if (overlap(s,t)) {
                    error("ranges %s %s overlap in lines %d, %d",
                        s, t, patterns[i]->line, patterns[index]->line);
                }
                if (subRange(s,t)) index = i;
            }
            else index = i;
        }
        if (index >= 0) {
            Pattern *range = patterns[index];
            delete(patterns, index);
            patterns = derange(patterns, range);
        }
    }
    return patterns;
}

// Expand all ranges in all states.
void derangeAll(State **states) {
    for (int i = 0; i < length(states); i++) {
        states[i]->patterns = derangeList(states[i]->patterns);
    }
}

// Compare pattern strings in lexicographic order, except that if s is a prefix
// of t, t comes before s (because the scanner tries longer patterns first). If
// patterns strings are equal, a pattern with a lookahead flag comes before one
// without. Otherwise, sort on the type.
int compare(Pattern *p, Pattern *q) {
    char *s = p->string, *t = q->string;
    if (prefix(s,t)) return 1;
    if (prefix(t,s)) return -1;
    int cmp = strcmp(s,t);
    if (cmp != 0) return cmp;
    if (p->look && ! q->look) return -1;
    if (! p->look && q->look) return 1;
    if (p->type < q->type) return -1;
    return 1;
}

void sort(Pattern **list) {
    int n = length(list);
    for (int i = 1; i < n; i++) {
        Pattern *p = list[i];
        int j = i - 1;
        while (j >= 0 && compare(list[j], p) > 0) {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = p;
    }
}

// Where a state has two or more patterns for the same string, add a soft flag
// to all except the last. Check that either the patterns are for close
// brackets, or that one is a lookahead which stays in the same state and has a
// type, and the other is a non-lookahead.
void addSoft(State *s) {
    for (int i = 0; i < length(s->patterns) - 1; i++) {
        Pattern *p = s->patterns[i];
        Pattern *q = s->patterns[i+1];
        if (! equal(p->string, q->string)) continue;
        bool ok = true;
        if (isCloser(p->type)) {
            if (! isCloser(q->type)) ok = false;
        }
        else {
            if (! p->look || q->look) ok = false;
            if (p->target != p->base) ok = false;
            if (p->type == More) ok = false;
        }
        p->soft = true;
        if (! ok) {
            if (p->line == q->line) {
                error("incompatible patterns on line %d", p->line);
            }
            error("incompatible patterns on lines %d, %d", p->line, q->line);
        }
     }
}

// Stage 5: expand ranges. Sort. Add soft flags. Optionally print.
void expandRanges(State **states, bool print) {
    derangeAll(states);
    for (int i = 0; i < length(states); i++) sort(states[i]->patterns);
    for (int i = 0; i < length(states); i++) addSoft(states[i]);
    if (print) for (int i=0; i < length(states); i++) printState(states[i]);
}

// ---------- Checks -----------------------------------------------------------
// Set flags to say if a state can occur at the start of tokens, or after the
// start, or both. Check that a state handles every individual character. Check
// that patterns for bracket types cannot be empty. Check that the scanner
// doesn't get stuck in an infinite loop. Give a warning for a lookahead past a
// newline, a space or newline in a longer token, a space with a type other
// than Gap, or a newline with a type other than Gap or a closer.

// Set the start and after flags deduced from a state's patterns. Return true
// if any changes were caused.
bool deduce(State *state) {
    bool changed = false;
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern *p = state->patterns[i];
        State *target = p->target;
        if (p->type != More) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->type == More && ! p->look) {
            if (! target->after) changed = true;
            target->after = true;
        }
        if (p->type == More && p->look && state->start) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->type == More && p->look && state->after) {
            if (! target->after) changed = true;
            target->after = true;
        }
    }
    return changed;
}

// Set the start & after flags for all states. Continue until no changes.
void deduceAll(State **states) {
    states[0]->start = true;
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < length(states); i++) {
            changed = changed || deduce(states[i]);
        }
    }
}

// Check that a state handles every singleton character.
void complete(State *state) {
    for (char ch = '\n'; ch <= '~'; ch++) {
        if (ch > '\n' && ch < ' ') continue;
        bool ok = false;
        for (int i = 0; i < length(state->patterns); i++) {
            char *s = state->patterns[i]->string;
            if (s[1] != '\0') continue;
            if (s[0] == ch) ok = true;
        }
        if (ok) continue;
        if (ch == ' ') error("state %s doesn't handle \\s", state->name);
        else if (ch == '\n') error("state %s doesn't handle \\n", state->name);
        else error("state %s doesn't handle %c", state->name, ch);
    }
}

// Check that patterns for brackets aren't empty, i.e. either the state has no
// start flag, or the pattern is a non-lookahead.
void checkBrackets(State *state) {
    if (! state->start) return;
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern *p = state->patterns[i];
        if (! p->look) continue;
        if (! isOpener(p->type) || ! isCloser(p->type)) continue;
        error("bracket type may have an empty token on line %d", p->line);
    }
}

// Search for a chain of (non-soft) lookaheads from a given state which can
// cause an infinite loop; look is the longest lookahead in the chain so far.
void follow(State **states, State *state, char *look) {
    if (state->visited) error("state %s can loop", state->name);
    state->visited = true;
    for (int i = 0; i < length(state->patterns); i++) {
        if (! state->patterns[i]->look || state->patterns[i]->soft) continue;
        char *s = state->patterns[i]->string;
        if (s[0] > look[0]) break;
        if (s[0] < look[0]) continue;
        char *next;
        if (prefix(s, look) || equal(s,look)) next = look;
        else if (prefix(look, s)) next = s;
        else continue;
        State *target = state->patterns[i]->target;
        follow(states, target, next);
    }
    state->visited = false;
}

// Start a search from a given state, for each possible input character.
void search(State **states, State *state) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if ('\n' < ch && ch < ' ') continue;
        char *look = singles[ch];
        follow(states, state, look);
    }
}

// Give a warning if a state has a pattern which looks ahead past a newline.
void warnNewline(State *state) {
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern *p = state->patterns[i];
        if (! p->look) continue;
        char *s = p->string;
        int sn = strlen(s);
        char *nl = strchr(s, '\n');
        if (nl == NULL) continue;
        if (nl == s + sn - 1) continue;
        printf("Warning: lookahead past newline on line %d\n", p->line);
        printf("(prevents simple line-base rescanning)\n");
    }
}

// Give a warning if a state has a non-lookahead pattern containing a space or
// newline with other characters.
void warnEmbed(State *state) {
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern *p = state->patterns[i];
        if (p->look) continue;
        char *s = p->string;
        if (strlen(s) == 1) continue;
        if (strchr(s, ' ') == NULL && strchr(s, '\n') == NULL) continue;
        printf("Warning: space or newline in token on line %d\n", p->line);
        printf("(prevents simple word-based motion or reformatting)\n");
    }
}

// Give a warning if a state matches a space or newline with a risk of
// including other characters. Also warn if given type other than gap or closer.
void warnInclude(State *state) {
    bool hasSpaceLookahead = false, hasNewlinelookahead = false;
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern *p = state->patterns[i];
        char *s = p->string;
        if (s[0] != ' ' && s[0] != '\n') continue;
        if (p->look) {
            if (s[0] == ' ' && p->soft) hasSpaceLookahead = true;
            if (s[0] == '\n' && p->soft) hasNewlinelookahead = true;
            continue;
        }
        if (p->type == More) {
            printf("Warning: space or newline");
            printf("with no type on line %d\n", p->line);
            printf("(risks being included in longer token)\n");
        }
        else if (s[0] == ' ' && p->type != Gap) {
            printf("Warning: space given non-Gap type on line %d\n", p->line);
        }
        else if (s[0] == '\n' && p->type != Gap && ! isCloser(p->type)) {
            printf("Warning: on line %d, newline given type", p->line);
            printf("which is not Gap or a closer (suffix E)\n");
        }
        if (! state->after) continue;
        if (s[0] == ' ' && hasSpaceLookahead) continue;
        if (s[0] == '\n' && hasNewlinelookahead) continue;
        printf("Warning: on line %d, space or newline matched\n", p->line);
        printf("with risk of adding it to a non-empty token.\n");
    }
}

// Stage 6: carry out checks. Optionally print.
void checkAll(State **states, bool print) {
    deduceAll(states);
    for (int i = 0; i < length(states); i++) {
        complete(states[i]);
        checkBrackets(states[i]);
        search(states, states[i]);
        warnNewline(states[i]);
        warnEmbed(states[i]);
        warnInclude(states[i]);
    }
    if (print) for (int i=0; i < length(states); i++) printState(states[i]);
}

// ---------- Compiling --------------------------------------------------------
// Compile the states into a compact transition table. The table has a row for
// each state, and an overflow area used when there is more than one pattern
// for a particular character. Each row consists of 96 cells of two bytes each,
// for \n and \s and !..~. The scanner uses the current state and the next
// character in the source text to look up a cell. The cell may be an action,
// i.e. a type and a target state, for that single character, or an offset
// relative to the start of the table to a list of patterns in the overflow
// area starting with that character, with their actions.

typedef unsigned char byte;
enum { COLUMNS = 96, CELL = 2 };

// Flags added to the type in a cell. The LINK flag in the main table indicates
// that the action is a link to the overflow area. The LOOK flag indicates a
// lookahead pattern. The SOFT flag, in the overflow area, represents one of
// two things. Without the LOOK flag, it represents a close bracket rule that
// is skipped if the bracket doesn't match the most recent unmatched open
// bracket. With the LOOK flag, it represents a lookahead which only applies if
// there is a non-empty current token.

enum { LINK = 0x80, SOFT = 0x80, LOOK = 0x40, FLAGS = 0xC0 };

// When there is more than one pattern for a state starting with a character,
// enter [LINK+hi, lo] where [hi,lo] is the offset to the overflow area.
void compileLink(byte *cell, int offset) {
    cell[0] = LINK | ((offset >> 8) & 0x7F);
    cell[1] = offset & 0xFF;
}

// Fill in an action other than a link: [SOFT+LOOK+type, target]
void compileAction(byte *action, Pattern *p) {
    int type = p->type;
    if (p->soft) type = SOFT | type;
    if (p->look) type = LOOK | type;
    action[0] = type;
    action[1] = p->target->row;
}

// Fill in a pattern in the overflow area and return the possibly moved table.
// The pattern is stored as a byte containing the length, followed by the
// characters of the pattern after the first, followed by the action. For
// example <= with type Op and target t is stored as 4 bytes [2, '=', Op, t].
byte *compileExtra(byte *table, Pattern *p) {
    int len = strlen(p->string);
    int n = length(table);
    table = adjust(table, len+2);
    table[n] = len;
    strncpy((char *)&table[n+1], p->string + 1, len - 1);
    compileAction(&table[n+len], p);
    return table;
}

// Compile a state into the table, and return the possibly moved table.
byte *compileState(byte *table, State *state) {
    Pattern **ps = state->patterns;
    int n = length(ps);
    char prev = '\0';
    for (int i = 0; i < n; i++) {
        Pattern *p = ps[i];
        char ch = p->string[0];
        int col = 0;
        if (ch != '\n') col = 1 + (ch - ' ');
        byte *cell = &table[CELL * (COLUMNS * state->row + col)];
        if (ch != prev) {
            prev = ch;
            bool direct = (i == n-1 || ch != ps[i+1]->string[0]);
            if (direct) compileAction(cell, p);
            else {
                compileLink(cell, length(table));
                table = compileExtra(table, p);
            }
        }
        else table = compileExtra(table, p);
    }
    return table;
}

// Stage 7: build the table.
byte *compile(State **states) {
    byte *table = newList(1);
    table = adjust(table, length(states) * COLUMNS * CELL);
    for (int i = 0; i < length(states); i++) {
        table = compileState(table, states[i]);
    }
    return table;
}

// ---------- Scanning ---------------------------------------------------------
// A line of source text is scanned to produce an array of bytes, one for each
// character. The first byte corresponding to a token gives its type (e.g. Id
// for an identifier). The bytes corresponding to the remaining characters of
// the token contain More. A stack is used for bracket matching.

// A scanner contains the transition table, the input, expected output and
// actual output, a stack of unmatched open brackets, the states and start and
// length for tracing.
struct scanner {
    byte *table; char *in, *expect; byte *out; int *stack;
    State **states; int trace, end;
};
typedef struct scanner Scanner;

// Brackets are marked with a flag if they are mismatched.
enum { MISMATCH = 0x80 };

// Create scanner.
Scanner *newScanner(byte *table, State **states) {
    Scanner *sc = newObject(sizeof(Scanner));
    sc->table = table;
    sc->in = newList(1);
    sc->expect = newList(1);
    sc->out = newList(1);
    sc->stack = newList(sizeof(int));
    sc->states = states;
    sc->trace = sc->end = -1;
    return sc;
}

// Check if a closer matches the top opener.
bool matchTop(Scanner *sc, int type) {
    int n = length(sc->stack);
    if (n == 0) return false;
    int top = sc->stack[n-1];
    return bracketMatch(sc->out[top], type);
}

// Push an opener.
void push(Scanner *sc, int opener) {
    sc->stack = adjust(sc->stack, +1);
    sc->stack[length(sc->stack) - 1] = opener;
}

// Pop an opener, and mark it and the closer as mismatched as appropriate.
void pop(Scanner *sc, int closer) {
    int n = length(sc->stack);
    int opener = -1;
    int left = -1;
    if (n > 0) {
        opener = sc->stack[n-1];
        adjust(sc->stack, -1);
        left = sc->out[opener];
    }
    int right = sc->out[closer];
    if (! bracketMatch(left, right)) {
        if (opener >= 0) sc->out[opener] = left | MISMATCH;
        sc->out[closer] = right | MISMATCH;
    }
}

// Print out one scanning step.
void trace(Scanner *sc, int state, bool look, int at, int n, int t) {
    char pattern[2*n+2];
    pattern[0] = '\0';
    if (look) strcat(pattern, "|");
    for (int i = 0; i < n; i++) {
        char ch = sc->in[at];
        if (ch == ' ') strcat(pattern, "\\s");
        else if (ch == '\n') strcat(pattern, "\\n");
        else if (ch == '\\') strcat(pattern, "\\\\");
        else if (ch == '|') strcat(pattern, "\\|");
        else strcat(pattern, (char[2]){ch,'\0'});
    }
    char *base = sc->states[state]->name;
    char *type = (t == More) ? "" : kindName(t);
    printf("%-10s %-10s %-10s\n", base, pattern, type);
}

// Use the given table and initial state to scan a source text.
void scan(Scanner *sc) {
    int n = length(sc->in);
    for (int i = 0; i < n; i++) sc->out[i] = More;
    int at = 0, start = 0, state = 0;
    while (at < n) {
        char ch = sc->in[at];
        int col = 0;
        if (ch != '\n') col = 1 + (ch - ' ');
        byte *action = &sc->table[CELL * (COLUMNS * state + col)];
        int len = 1;
        if ((action[0] & LINK) != 0) {
            int offset = ((action[0] & 0x7F) << 8) + action[1];
            byte *p = sc->table + offset;
            bool found = false;
            while (! found) {
                found = true;
                len = p[0];
                for (int i = 1; i < len && found; i++) {
                    if (sc->in[at + i] != p[i]) found = false;
                }
                bool look = p[len] & LOOK;
                bool soft = p[len] & SOFT;
                int type = p[len] & ~FLAGS;
                if (found && soft) {
                    if (! look && ! matchTop(sc,type)) found = false;
                    if (look && start == at) found = false;
                }
                if (found) action = p + len;
                else p = p + len + 2;
            }
        }
        bool look = (action[0] & LOOK) != 0;
        int type = action[0] & ~FLAGS;
        int target = action[1];
        if (sc->trace <= at && at < sc->end) {
            trace(sc, state, look, at, len, type);
        }
        if (! look) at = at + len;
        if (type != More && start < at) {
            sc->out[start] = type;
            if (isOpener(type)) push(sc, start);
            else if (isCloser(type)) pop(sc, start);
            start = at;
        }
        state = target;
    }
}

// ---------- Testing ----------------------------------------------------------
// The tests in a language description are intended to check that the rules work
// as expected. They also act as tests for this program. A line starting with >
// is a test and one immediately following and starting with < is the expected
// output.

// Extract the tests and expected output from the source lines. Add a newline to
// a test line. If the expected output line lacks a character for the type of
// the newline, add a space.
void extract(Scanner *sc, char **lines) {
    for (int i = 0; i < length(lines); i++) {
        if (lines[i][0] != '>') continue;
        if (i == length(lines) - 1 || lines[i+1][0] != '<') {
            error("test without expected output on line %d", i+1);
        }
        int p = length(sc->in);
        int n = strlen(lines[i]);
        sc->in = adjust(sc->in, n);
        strncpy(&sc->in[p], lines[i]+1, n-1);
        sc->in[p+n-1] = '\n';
        int n1 = strlen(lines[i+1]);
        if (n1 < n || n1 > n+1) {
            error("expected output has wrong length on line %d", i+1);
        }
        sc->expect = adjust(sc->expect, n);
        strncpy(&sc->expect[p], lines[i+1]+1, n1);
        if (n1 == n) sc->expect[p+n-1] = ' ';

    }
    sc->out = adjust(sc->out, length(sc->in));
}

// Translate the output bytes to characters, in place. An ordinary type becomes
// an upper case letter, the first letter of its name, and mismatched or
// unmatched brackets become lower case.
char *translate(byte *out) {
    char *tr = (char *) out;
    for (int i = 0; i < length(out); i++) {
        byte b = out[i];
        char ch = visualKind(b & ~MISMATCH);
        if ((b & MISMATCH) != 0) ch = tolower(ch);
        tr[i] = ch;
    }
    return tr;
}

// Check the scanner output against the expected output. Set the start and end
// indexes of a failed test. Report it. Return success or failure.
bool checkResults(Scanner *sc) {
    char *out = translate(sc->out);
    int fail = -1;
    for (int i = 0; i < length(sc->in) && fail < 0; i++) {
        if (out[i] == sc->expect[i]) continue;
        fail = i;
    }
    if (fail < 0) return true;
    for (sc->trace = fail; sc->trace >= 0; sc->trace--) {
        if (sc->trace == 0 || sc->in[sc->trace - 1] == '\n') break;
    }
    for (sc->end = sc->trace + 1; ; sc->end++) {
        if (sc->in[sc->end-1] == '\n') break;
    }
    printf("Test failed. The input, expected output, "
        "actual output, and trace are:\n\n");
    printf(">%.*s\n", sc->end - sc->trace - 1, &sc->in[sc->trace]);
    printf("<%.*s\n", sc->end - sc->trace, &sc->expect[sc->trace]);
    printf("<%.*s\n\n", sc->end - sc->trace, &out[sc->trace]);
    return false;
}

// Stage 8: Run the tests and check the results. If there is a failure, run the
// tests again with tracing switched on for the failed test.
bool runTests(Scanner *sc, char **lines) {
    extract(sc, lines);
    scan(sc);
    bool ok = checkResults(sc);
    if (ok) return true;
    clear(sc->stack);
    scan(sc);
    return false;
}

// ---------- Main -------------------------------------------------------------
// Run all the stages. On success, write out the table.

void write(char *path, byte *table) {
    FILE *p = fopen(path, "wb");
    fwrite(table, length(table), 1, p);
    fclose(p);
}

int main(int n, char *args[n]) {
    if (n != 2) error("usage: compile lang.txt");
    char *path = args[1];
    bool txt = strcmp(path + strlen(path) - 4, ".txt") == 0;
    if (! txt) error("expecting extension .txt");
    char **lines = getLines(path);
    Rule **rules = getRules(lines);
    State **states = getStates(rules, false);
    getPatterns(rules, states, false);
    expandRanges(states, false);
    checkAll(states, false);
    byte *table = compile(states);
    Scanner *sc = newScanner(table, states);
    bool ok = runTests(sc, lines);
    if (! ok) return 1;
    char outpath[strlen(path)+1];
    strcpy(outpath, path);
    strcpy(outpath + strlen(path) - 4, ".bin");
    write(outpath, table);
    printf("Tests passed, file %s written\n", outpath);
    freeAll();
}
