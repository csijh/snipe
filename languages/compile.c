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

// Use the type constants from the main editor.
#include "../src/types.h"

/*
// ---------- types ------------------------------------------------------------
// These types and their names must be kept the same as in src/scan.h.
// A type is used to mark a text character, to represent the result of
// scanning. The bracket types come in matching pairs, with a B or E suffix.
// A few types and flags are used internally:
//   None     means no type, and marks token characters after the first
//   Gap      marks a space or spaces as a separator
//   Bad      flags a token, reversibly, as mismatched
// For display, XB -> X, XNB -> X, ... X+Comment -> Note, X+Bad -> Wrong.

enum type {
    None, Gap, Newline, Alternative, Comment, Declaration, Function, Identifier,
    Join, Keyword, Long, Mark, NoteD, Note, Operator, Property, QuoteD, Quote,
    Tag, Unary, Value, Wrong,

    LongB, CommentB, CommentNB, TagB, RoundB, Round2B, SquareB, Square2B,
    GroupB, Group2B, BlockB, Block2B,

    LongE, CommentE, CommentNE, TagE, RoundE, Round2E, SquareE, Square2E,
    GroupE, Group2E, BlockE, Block2E,

    FirstB = LongB, LastB = Block2B, FirstE = LongE, LastE = Block2E,
    Bad = 128,
};

// The full names of the types.
char *typeNames[64] = {
    [None]="None", [Gap]="Gap", [Newline]="Newline",
    [Alternative]="Alternative", [Comment]="Comment",
    [Declaration]="Declaration", [Function]="Function",
    [Identifier]="Identifier", [Join]="Join", [Keyword]="Keyword",
    [Long]="Long", [Mark]="Mark", [NoteD]="NoteD", [Note]="Note",
    [Operator]="Operator", [Property]="Property", [QuoteD]="QuoteD",
    [Quote]="Quote", [Tag]="Tag", [Unary]="Unary", [Value]="Value",
    [Wrong]="Wrong",

    [LongB]="LongB", [CommentB]="CommentB", [CommentNB]="CommentNB",
    [TagB]="TagB", [RoundB]="RoundB", [Round2B]="Round2B", [SquareB]="SquareB",
    [Square2B]="Square2B", [GroupB]="GroupB", [Group2B]="Group2B",
    [BlockB]="BlockB", [Block2B]="Block2B",

    [LongE]="LongE", [CommentE]="CommentE", [CommentNE]="CommentNE",
    [TagE]="TagE", [RoundE]="RoundE", [Round2E]="Round2E", [SquareE]="SquareE",
    [Square2E]="Square2E", [GroupE]="GroupE", [Group2E]="Group2E",
    [BlockE]="BlockE", [Block2E]="Block2E" };

char visualType(int type) {
    switch (type) {
    case None: return '-';
    case Gap: return ' ';
    case Newline: return '.';
    default: return typeNames[type][0];
    }
}

bool isOpener(int type) {
    return FirstB <= type && type <= LastB;
}

bool isCloser(int type) {
    return FirstE <= type && type <= LastE;
}

bool bracketMatch(int open, int close) {
    return close == open + FirstE - FirstB;
}
*/
bool equal(char *s, char *t) {
    return strcmp(s, t) == 0;
}

bool prefix(char *s, char *t) {
    if (strlen(s) > strlen(t)) return false;
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

// Convert a string to a type. Handle suffixes and abbreviations.
int findType(char *s) {
    for (int i = Alternative; i <= Block2E; i++) {
        char *name = typeName(i);
        if (equal(s, name)) return i;
        if (! islower(name[strlen(name)-1])) continue;
        if (prefix(s, name)) return i;
    }
    return -1;
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
// A pattern is like a rule from the source text, but with just a single pattern
// string. A pattern has a line number, base and target states, a lookahead
// flag, a soft flag, an unescaped pattern string, and a type. A pattern string
// is unescaped by transferring an initial backslash to the lookahead flag,
// replacing a double backslash by a single backslash, replacing \s and \n by
// an actual space or newline (printed as S or N) and replacing \ on its own by
// two ranges N..N and S..~. The N..N is kept as a range at this stage, to
// indicate that it can be overridden by a \n rule.

struct pattern {
    int line; State *base, *target; bool look, soft; char *string; int type;
};
typedef struct pattern Pattern;

// Deal with the escape conventions in the most recently added pattern.
//   \\\... -> look \...
//   \\...  -> \...
//   \s     -> look SP
//   \n     -> look NL
//   \x     -> error
//   \...   -> look ...
//   \      -> look NL..NL, and look SP..~
Pattern **unescape(Pattern **patterns) {
    int n = length(patterns);
    Pattern *p = patterns[n-1];
    char *s = p->string;
    char sn = strlen(s);
    if (prefix("\\\\\\", s)) { p->look = true; p->string = &s[2]; }
    else if (prefix("\\\\", s)) { p->string  = &s[1]; }
    else if (equal(s, "\\s")) { p->look = true; p->string = " "; }
    else if (equal(s, "\\n")) { p->look = true; p->string = "\n"; }
    else if (prefix("\\",s) && sn >= 2) { p->look = true; p->string = &s[1]; }
    else if (equal(s,"\\")) {
        p->look = true;
        p->string = "!..~";
        Pattern *extra1 = newObject(sizeof(Pattern));
        Pattern *extra2 = newObject(sizeof(Pattern));
        *extra1 = *p;
        *extra2 = *p;
        extra1->string = "\n..\n";
        extra2->string = " .. ";
        patterns = adjust(patterns, +2);
        patterns[n] = extra1;
        patterns[n+1] = extra2;
    }
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
    int type = None;
    if (isupper(last[0])) {
        type = findType(last);
        if (type < 0) error("unknown type on line %d", line);
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
        patterns = unescape(patterns);
    }
    return patterns;
}

// Display a pattern.
void printPattern(Pattern *p) {
    printf("%-10s ", p->base->name);
    char *s = p->string;
    if (p->look && strcmp(s," ") == 0) printf("%-14s ", "\\s");
    else if (p->look && strcmp(s,"\n") == 0) printf("%-14s ", "\\n");
    else if (p->look && strcmp(s,"\n..\n") == 0) printf("%-14s ", "\\n..n");
    else if (p->look && strcmp(s," .. ") == 0) printf("%-14s ", "\\s..s");
    else if (p->look && s[0] == '\\') printf("\\\\\\%-11s ", s);
    else if (p->look || s[0] == '\\') printf("\\%-13s ", s);
    else printf("%-14s ", s);
    printf("%-10s ", p->target->name);
    if (p->type != None) printf("%-10s", typeName(p->type));
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

// Print a state's patterns. After range expansion and sorting, a state may have
// runs of many one-character patterns. Gather a run and print it as a range.
void printState(State *state) {
    Pattern **ps = state->patterns;
    for (int i = 0; i < length(ps); i++) {
        int j = i;
        while (j < length(ps)-1 && compatible(ps[j], ps[j+1])) j++;
        if (j == i) printPattern(ps[i]);
        else {
            Pattern range = *ps[i];
            char string[7];
            sprintf(string, "%c..%c", ps[i]->string[0], ps[j]->string[0]);
            range.string = string;
            printPattern(&range);
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
// Expand ranges such as 0..9 to multiple one-character patterns, with more
// specific patterns (subranges and individual characters) taking precedence.
// Then sort the patterns in each state. Then add a soft flag for a close
// bracket pattern if it is followed by another close bracket pattern for the
// same pattern string.

// Single character strings covering \n \s !..~ for expanding ranges.
char *singles[128] = {
    "?","?","?", "?","?","?","?","?","?","?","\n","?","?", "?","?","?",
    "?","?","?", "?","?","?","?","?","?","?","?", "?","?", "?","?","?",
    " ","!","\"","#","$","%","&","'","(",")","*", "+",",", "-",".","/",
    "0","1","2", "3","4","5","6","7","8","9",":", ";","<", "=",">","?",
    "@","A","B", "C","D","E","F","G","H","I","J", "K","L", "M","N","O",
    "P","Q","R", "S","T","U","V","W","X","Y","Z", "[","\\","]","^","_",
    "`","a","b", "c","d","e","f","g","h","i","j", "k","l", "m","n","o",
    "p","q","r", "s","t","u","v","w","x","y","z", "{","|", "}","~","?"
};

bool isRange(char *s) {
    if (strlen(s) != 4) return false;
    if (s[1] != '.' || s[2] != '.') return false;
    return true;
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

// Expand all ranges in all states. Optionally print.
void derangeAll(State **states) {
    for (int i = 0; i < length(states); i++) {
        states[i]->patterns = derangeList(states[i]->patterns);
    }
}

// Compare strings in lexicographic order, except that if s is a prefix of t, t
// comes before s (because the scanner tries longer patterns first).
int compare(char *s, char *t) {
    if (prefix(s,t)) return 1;
    if (prefix(t,s)) return -1;
    return strcmp(s,t);
}

void sort(Pattern **list) {
    int n = length(list);
    for (int i = 1; i < n; i++) {
        Pattern *p = list[i];
        int j = i - 1;
        while (j >= 0 && compare(list[j]->string, p->string) > 0) {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = p;
    }
}

void sortAll(State **states) {
    for (int i = 0; i < length(states); i++) {
        State *state = states[i];
        sort(state->patterns);
    }
}

// Where a state has a pattern involving a close bracket, and it is followed
// by another close bracket pattern for the same string, add a soft flag.
// For example,
//    s } t BlockE (soft)
//    s } u GroupE
void addSoft(State *s) {
    for (int i = 0; i < length(s->patterns); i++) {
        Pattern *p = s->patterns[i];
        if (! isCloser(p->type)) continue;
        bool last = false;
        if (i == length(s->patterns) - 1) last = true;
        else if (! equal(p->string, s->patterns[i+1]->string)) last = true;
        else if (! isCloser(s->patterns[i+1]->type)) last = true;
        if (! last) p->soft = true;
     }
}

// Stage 5: expand ranges. Sort. Add soft flags. Optionally print.
void expandRanges(State **states, bool print) {
    derangeAll(states);
//    sortAll(states);
    for (int i = 0; i < length(states); i++) sort(states[i]->patterns);
    for (int i = 0; i < length(states); i++) addSoft(states[i]);
    if (print) for (int i=0; i < length(states); i++) printState(states[i]);
}

// ---------- Checks -----------------------------------------------------------
// Check that a scanner handles every input unambiguously. Check whether states
// occur at the start of tokens, or after the start, or both. Check that if
// after, \s \n patterns have types. Check that the scanner doesn't get stuck in
// an infinite loop.

// Set the start and after flags deduced from a state's patterns. Return true
// if any changes were caused.
bool deduce(State *state) {
    bool changed = false;
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern *p = state->patterns[i];
        State *target = p->target;
        if (p->type != None) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->type == None && ! p->look) {
            if (! target->after) changed = true;
            target->after = true;
        }
        if (p->type == None && p->look && state->start) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->type == None && p->look && state->after) {
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

// Check that a state has no duplicate patterns, except for stack pops.
void noDuplicates(State *state) {
    Pattern **list = state->patterns;
    for (int i = 0; i < length(list); i++) {
        Pattern *p = list[i];
        char *s = p->string;
        for (int j = i+1; j < length(list); j++) {
            Pattern *q = list[j];
            char *t = q->string;
            if (! equal(s,t)) continue;
            if (isCloser(p->type) && isCloser(q->type) && p->type != q->type) {
                continue;
            }
            error("state %s has pattern for %s twice", state->name, s);
        }
    }
}

// Check that a state handles every singleton character.
void complete(State *state) {
    char ch = '\n';
    for (int i = 0; i < length(state->patterns); i++) {
        char *s = state->patterns[i]->string;
        if (s[1] != '\0') continue;
        if (s[0] == ch) {
            if (ch == '\n') ch = ' ';
            else ch = ch + 1;
        }
    }
    if (ch > '~') return;
    if (ch == ' ') error("state %s doesn't handle \\s", state->name);
    else if (ch == '\n') error("state %s doesn't handle \\n", state->name);
    else error("state %s doesn't handle %c", state->name, ch);
}

// Check that bracket types are not associated with lookaheads, except for
// QuoteE with \n.
void checkBrackets(State *base) {
    for (int i = 0; i < length(base->patterns); i++) {
        Pattern *p = base->patterns[i];
        if (! p->look) continue;
        if (! isOpener(p->type) && ! isCloser(p->type)) continue;
        if (p->type == QuoteE && p->string[0] == '\n') continue;
        error("bracket type with lookahead on line %d", p->line);
    }
}

// Check, for a state with the after flag, that \s \n patterns have types.
void separates(State *state) {
    if (! state->after) return;
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern *p = state->patterns[i];
        if (p->string[0] == ' ' || p->string[0] == '\n') {
            if (p->type == None) {
                error(
                    "state %s should terminate tokens on matching \\s or \\n",
                    state->name
                );
            }
        }
    }
}

// Search for a chain of lookaheads from a given state, other than \s or \n,
// which can cause an infinite loop. The look argument is the longest lookahead
// in the chain so far.
void follow(State **states, State *state, char *look) {
    if (state->visited) error("state %s can loop", state->name);
    state->visited = true;
    for (int i = 0; i < length(state->patterns); i++) {
        if (! state->patterns[i]->look) continue;
        char *s = state->patterns[i]->string;
        if (s[0] == ' ' || s[0] == '\n') continue;
        if (s[0] > look[0]) break;
        if (s[0] < look[0]) continue;
        char *next;
        if (prefix(s, look)) next = look;
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

// Stage 6: carry out checks. Optionally print.
void checkAll(State **states, bool print) {
    deduceAll(states);
    for (int i = 0; i < length(states); i++) {
        noDuplicates(states[i]);
        complete(states[i]);
        checkBrackets(states[i]);
        separates(states[i]);
        search(states, states[i]);
    }
    if (print) for (int i=0; i < length(states); i++) printState(states[i]);
}

// ---------- Compiling --------------------------------------------------------
// Compile the states into a compact transition table. The table has a row for
// each state, and an overflow area used when there is more than one pattern
// for a particular character. Each row consists of 96 entries of two bytes
// each, one for each character \n, \s, !, ..., ~. The scanner uses the current
// state and the next character in the source text to look up an entry. The
// entry may be an action for that single character, or an offset relative to
// the start of the table to a list of patterns in the overflow area starting
// with that character, with their actions.

typedef unsigned char byte;

// Flags added to the type in an action. The LINK flag in the main table
// indicates that the action is a link to the overflow area. The SOFT flag, in
// the overflow area, represents a close bracket rule that is skipped if the
// bracket doesn't match the most recent unmatched open bracket. The LOOK flag
// indicates a lookahead pattern. The TYPE mask removes the two flags.
enum { LINK = 0x80, SOFT = 0x80, LOOK = 0x40, TYPE = 0x3F };

// When there is more than one pattern for a state starting with a character,
// enter [LINK+hi, lo] where [hi,lo] is the offset to the overflow area.
void compileLink(byte *action, int offset) {
    action[0] = LINK | ((offset >> 8) & 0x7F);
    action[1] = offset & 0xFF;
}

// Fill in an action other than a link: [SOFT+LOOK+type, target]
void compileAction(byte *action, Pattern *p) {
    int type = p->type;
    if (p->look) type = LOOK | type;
    if (p->soft) type = SOFT | type;
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
        int col = (ch == '\n') ? 0 : ch - ' ' + 1;
        byte *entry = &table[2 * (96 * state->row + col)];
        if (ch != prev) {
            prev = ch;
            bool direct = (i == n-1 || ch != ps[i+1]->string[0]);
            if (direct) compileAction(entry, p);
            else {
                compileLink(entry, length(table));
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
    table = adjust(table, 2 * 96 * length(states));
    for (int i = 0; i < length(states); i++) {
        table = compileState(table, states[i]);
    }
    return table;
}

// ---------- Scanning ---------------------------------------------------------
// A line of source text is scanned to produce an array of bytes, one for each
// character. The first byte corresponding to a token gives its type (e.g. Id
// for an identifier). The bytes corresponding to the remaining characters of
// the token contain None.

// Bracket matching is simulated by marking bytes in the output array with 0x80
// for matched brackets, 0x40 for mismatched brackets, and 0xC0 for unmatched
// open brackets.
enum { MATCH = 0x80, MISMATCH = 0x40, OPEN = 0xC0, FLAGS = 0xC0 };

// A tracer is an object containing the states (for their names) and the text of
// the traced execution.
struct tracer { State **states; char *text; };
typedef struct tracer Tracer;

bool matchTop(int type, byte *out, int at) {
    for (int i = at-1; i >= 0; i--) {
        if ((out[i] & FLAGS) == OPEN) return bracketMatch((out[i] & TYPE), type);
    }
    return false;
}

// Simulate a pushed open bracket.
void push(byte *out, int at) {
    out[at] = out[at] | OPEN;
}

// Simulate popping. Find the 'top' and match or mismatch.
void pop(byte *out, int at) {
    int open = -1;
    for (int i = at-1; i >= 0; i--) {
        if ((out[i] & FLAGS) == OPEN) open = i;
    }
    int left = out[open] & TYPE;
    int right = out[at] & TYPE;
    if (bracketMatch(left, right)) {
        if (open >= 0) out[open] = left | MATCH;
        out[at] = right | MATCH;
    }
    else {
        if (open >= 0) out[open] = left | MISMATCH;
        out[at] = right | MISMATCH;
    }
}

// Add a string to the end of a dynamic string.
char *addString(char *ds, char *s) {
    int n = length(ds);
    ds = adjust(ds, strlen(s));
    strcpy(&ds[n-1], s);
    return ds;
}

void trace(int r, bool l, char *in, int at, int n, int t, Tracer *tracer) {
    char pattern[n+3];
    pattern[0] = '\0';
    if (l) strcat(pattern, "\\");
    if (in[at] == '\\') strcat(pattern, "\\");
    for (int i = 0; i < n; i++) {
        char ch[2] = "?";
        ch[0] = in[at+i];
        if (ch[0] == ' ') ch[0] = 's';
        if (ch[0] == '\n') ch[0] = 'n';
        strcat(pattern, ch);
    }
    char line[100] = "";
    char *base = tracer->states[r]->name;
    char *type = (t == None) ? "" : typeName(t);
    sprintf(line, "%-10s %-10s %-10s\n", base, pattern, type);

    tracer->text = addString(tracer->text, line);
    if (at[in] == '\n') tracer->text = addString(tracer->text, "\n");
}

// Use the given table and start row to scan the given input line, producing
// the result in the given byte array, and returning the final state.
int scan(byte *table, int row, char *in, byte *out, Tracer *tracer) {
    int n = strlen(in);
    for (int i = 0; i < n; i++) out[i] = None;
    int at = 0, start = 0;
    while (at < n) {
        char ch = in[at];
        int col = (ch == '\n') ? 0 : ch - ' ' + 1;
        byte *action = &table[2 * (96 * row + col)];
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
                int t = p[len] & TYPE;
                if (found) {
                    bool soft = (p[len] & SOFT) != 0;
                    if (soft && ! matchTop(t,out,at)) found = false;
                }
                if (found) action = p + len;
                else p = p + len + 2;
            }
        }
        bool lookahead = (action[0] & LOOK) != 0;
        int type = action[0] & TYPE;
        int target = action[1];
        trace(row, lookahead, in, at, len, type, tracer);
        if (! lookahead) at = at + len;
        if (type != None && start < at) {
            int type2 = type;
            if (ch == '\n' && type == QuoteE) type2 = Quote;
            out[start] = type2;
            if (isOpener(type2)) push(out, start);
            else if (isCloser(type2)) pop(out, start);
            start = at;
        }
        if (ch == ' ') { out[at++] = Gap; start = at; }
        else if (ch == '\n' && type == QuoteE) {
            out[at++] = Quote2E;
            pop(out, at-1);
            start = at;
        }
        else if (ch == '\n') { out[at++] = Gap; start = at; }
        row = target;
    }
    return row;
}

// ---------- Testing ----------------------------------------------------------
// The tests in a language description are intended to check that the rules work
// as expected. They also act as tests for this program. A line starting with >
// is a test and one immediately following and starting with < is the expected
// output.

// Extract the tests from the source lines, as a single string with newlines.
char *extractTests(char **lines) {
    char *tests = newList(1);
    for (int i = 0; i < length(lines); i++) {
        if (lines[i][0] != '>') continue;
        int n = strlen(lines[i]);
        int to = length(tests);
        tests = adjust(tests, +n);
        strcpy(&tests[to], lines[i]+1);
        tests[to+n-1] = '\n';
    }
    int n = length(tests);
    tests = adjust(tests, +1);
    tests[n] = '\0';
    return tests;
}

// Extract expected outputs, make sure they line up with the tests.
char *extractExpected(char *tests, char **lines) {
    char *expected = newList(1);
    for (int i = 0; i < length(lines); i++) {
        if (lines[i][0] != '<') continue;
        int n = strlen(lines[i]);
        int to = length(expected);
        expected = adjust(expected, +n);
        strncpy(&expected[to], lines[i]+1, n);
        int at = length(expected);
        if (at > length(tests) || tests[at-1] != '\n') {
            error("output doesn't line up on line %d", i+1);
        }
    }
    int n = length(expected);
    expected = adjust(expected, +1);
    expected[n] = '\0';
    if (length(expected) != length(tests)) {
        error("test without output");
    }
    return expected;
}

// Translate the output bytes to characters, in place. An ordinary type becomes
// an upper case letter, the first letter of its name, and mismatched or
// unmatched brackets become lower case.
char *translate(byte *out) {
    char *tr = (char *) out;
    for (int i = 0; i < length(out)-1; i++) {
        byte b = out[i];
        char ch = visualType(b & TYPE);
        if ((b & FLAGS) == MISMATCH || (b & FLAGS) == OPEN) ch = tolower(ch);
        tr[i] = ch;
    }
    return tr;
}

int checkResults(char *tests, char *expected, char *out, Tracer *tracer) {
    int start = 0, end = 0, sT = 0, eT = 0;
    char *text = tracer->text;
    int count = 0;
    while (tests[end] != '\0') {
        while (end == start || tests[end-1] != '\n') end++;
        while (eT < sT+2 || text[eT-1] != '\n' || text[eT-2] != '\n') eT++;
        for (int i = start; i < end-1; i++) {
            if (expected[i] != (char)out[i]) {
                printf("Test failed. "
                    "Input, expected output and actual output are:\n");
                printf("%.*s\n", end-start-1, &tests[start]);
                printf("%.*s\n", end-start-1, &expected[start]);
                printf("%.*s\n", end-start-1, &out[start]);
                printf("\nTRACE:\n");
                printf("%.*s", eT-sT, &text[sT]);
                exit(1);
            }
        }
        start = end;
        sT = eT;
        count++;
    }
    return count;
}

void write(char *path, byte *table) {
    FILE *p = fopen(path, "wb");
    fwrite(table, length(table), 1, p);
    fclose(p);
}

// Stage 8: handle the tests (all at once). On success, write out the table.
// On failure, write out the trace of the failed test.
void runTests(char **lines, byte *table, State **states, char *path) {
    char *tests = extractTests(lines);
    char *expected = extractExpected(tests, lines);
    byte *out = newList(1);
    out = adjust(out, length(tests));
    Tracer *tracer = newObject(sizeof(Tracer));
    tracer->states = states;
    tracer->text = newList(1);
    adjust(tracer->text, +1);
    tracer->text[0] = '\0';
    scan(table, 0, tests, out, tracer);
    int n = checkResults(tests, expected, translate(out), tracer);
    char outpath[strlen(path)+1];
    strcpy(outpath, path);
    strcpy(outpath + strlen(path) - 4, ".bin");
    write(outpath, table);
    printf("%d tests passed, file %s written\n", n, outpath);
}

// ---------- Main -------------------------------------------------------------
// Run all the stages.

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
    runTests(lines, table, states, path);
    freeAll();
}
