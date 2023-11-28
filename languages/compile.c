// Free and open source, see licence.txt.

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

// TODO: push and pop now simpler. Still need a flag for 'last'.

// ---------- types ------------------------------------------------------------
// These types and their names must be kept the same as in other Snipe modules.
// A type is used to mark a text character, to represent the result of
// scanning. The bracket types come in matching pairs, with a B or E suffix.
// A few types and flags are used internally:
//   None     means no type, and marks token characters after the first
//   Gap      marks a space or spaces as a separator
//   Newline  marks a newline as a separator
//   Miss     a close bracket which forces a mismatch
//   Comment  flags a token, reversibly, as a comment
//   Bad      flags a token, reversibly, as mismatched
// For display, XB -> X, XNB -> X, ... X+Comment -> Note, X+Bad -> Wrong.

enum type {
    None, Gap, Newline, Miss, Alternative, Declaration, Function,
    Identifier, Join, Keyword, Long, Mark, Note, Operator, Property, Quote,
    Tag, Unary, Value, Wrong,

    QuoteB, LongB, NoteB, CommentB, CommentNB, TagB, RoundB, Round2B,
    SquareB, Square2B, GroupB, Group2B, BlockB, Block2B,

    QuoteE, LongE, NoteE, CommentE, CommentNE, TagE, RoundE, Round2E,
    SquareE, Square2E, GroupE, Group2E, BlockE, Block2E,

    Comment = 64, Bad = 128,
};

// The full names of the types. The first character is used in tests.
char *typeNames[64] = {
    "-", " ", ".", "Miss", "Alternative", "Declaration", "Function",
    "Identifier", "Join", "Keyword", "Long", "Mark", "Note", "Operator",
    "Property", "Quote", "Tag", "Unary", "Value", "Wrong",

    "QuoteB", "LongB", "NoteB", "CommentB", "CommentNB", "TagB", "RoundB",
    "Round2B", "SquareB", "Square2B", "GroupB", "Group2B", "BlockB", "Block2B",

    "QuoteE", "LongE", "NoteE", "CommentE", "CommentNE", "TagE", "RoundE",
    "Round2E", "SquareE", "Square2E", "GroupE", "Group2E", "BlockE", "Block2E",
};

bool pushType(int type) {
    return QuoteB <= type && type <= Block2B;
}

bool popType(int type) {
    return QuoteE <= type && type <= Block2E;
}

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
    if (equal(s,"None")) return None;
    for (int i = Alternative; i <= Block2E; i++) {
        char *name = typeNames[i];
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

struct header { short length, max, unit, index; void *align[]; };
typedef struct header header;

// Keep a global list of all allocations, for easy freeing.
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

// ---------- Patterns ---------------------------------------------------------
// A pattern is like a rule from the source text, but with just a single pattern
// string. A pattern has a line number, a base state, a lookahead flag, an
// unescaped pattern string, a target and a type. For later use, it has the
// index numbers of the base, target and type. A pattern string is unescaped by
// transferring an initial backslash to the lookahead flag, replacing a double
// backslash by a single backslash, replacing \s and \n by an actual space or
// newline (printed as S or N) and replacing \ on its own by two ranges N..N
// and S..~. The N..N is kept as a range at this stage, to indicate that it can
// be overridden by a \n rule.

struct pattern {
    int line; char *baseName; bool look; char *string, *targetName, *typeName;
    int base, target, type;
};
typedef struct pattern Pattern;

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
    else if (prefix("\\",s) && sn > 2) { p->look = true; p->string = &s[1]; }
    else if (prefix("\\",s) && sn == 2) {
        error("bad lookahead %s on line %d", s, p->line);
    }
    else if (equal(s,"\\")) {
        p->look = true;
        p->string = " ..~";
        Pattern *extra = newObject(sizeof(Pattern));
        *extra = *p;
        extra->string = "\n..\n";
        patterns = adjust(patterns, +1);
        patterns[n] = extra;
    }
    return patterns;
}

// Collect patterns from a rule in the source text.
Pattern **collectPatterns(Pattern **patterns, int line, char **strings) {
    int n = length(strings);
    if (n < 3) error("incomplete rule on line %d", line);
    char *base = strings[0];
    char *last = strings[n-1];
    char *type = "None";
    if (isupper(last[0])) {
        type = last;
        n--;
        if (n < 2) error("incomplete rule on line %d", line);
    }
    char *target = strings[n-1];
    if (! islower(target[0])) error("expecting target on line %d", line);
    for (int i = 1; i < n-1; i++) {
        char *string = strings[i];
        int r = length(patterns);
        patterns = adjust(patterns, +1);
        patterns[r] = newObject(sizeof(Pattern));
        *patterns[r] = (Pattern) {
            .line = line, .baseName = base, .look = false, .string = string,
            .targetName = target, .typeName = type
        };
        patterns = unescape(patterns);
    }
    return patterns;
}

// Display a pattern.
void printPattern(Pattern *p) {
    printf("%-10s ", p->baseName);
    if (p->look) printf("\\ "); else printf("  ");
    char s[20];
    strcpy(s, p->string);
    if (s[0] == ' ') s[0] = 'S';
    if (s[0] == '\n') s[0] = 'N';
    if (s[3] == ' ') s[3] = 'S';
    if (s[3] == '\n') s[3] = 'N';
    printf("%-14s ", s);
    printf("%-10s ", p->targetName);
    if (! equal(p->typeName,"None")) printf("%-10s", p->typeName);
    printf("\n");
}

// Stage 2: collect all the patterns from the rules, optionally print them.
Pattern **getPatterns(char **lines, bool print) {
    Pattern **patterns = newList(sizeof(Pattern *));
    for (int i = 0; i < length(lines); i++) {
        if (! islower(lines[i][0])) continue;
        char **strings = splitStrings(lines[i]);
        patterns = collectPatterns(patterns, i+1, strings);
    }
    if (print) {
        for (int i=0; i<length(patterns); i++) printPattern(patterns[i]);
    }
    return patterns;
}

// ---------- States -----------------------------------------------------------
// A state has a row number (its row in the final table), a name, and an array
// of patterns. It has flags to say whether it occurs at or after the start of
// tokens. It has a visited flag used when checking for infinite loops. It has
// a partner state if it begins with both start and after flags set, and so
// gets split into a pair.

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

// Create empty states from the patterns. Add base row numbers to patterns.
State **makeStates(Pattern **patterns) {
    State **states = newList(sizeof(State *));
    for (int i = 0; i < length(patterns); i++) {
        Pattern *p = patterns[i];
        int row = findState(states, p->baseName);
        if (row < 0) {
            row = length(states);
            states = addState(states, p->baseName);
        }
        p->base = row;
    }
    return states;
}

// Fill in the target rows and type codes for all the patterns
void finishPatterns(Pattern **patterns, State **states) {
    for (int i = 0; i < length(patterns); i++) {
        Pattern *p = patterns[i];
        p->target = findState(states, p->targetName);
        if (p->target < 0) {
            error("unknown target %s on line %d", p->targetName, p->line);
        }
        p->type = findType(p->typeName);
        if (p->type < 0) {
            error("unknown type %s on line %d", p->typeName, p->line);
        }
    }
}

// Distribute the patterns into the states.
void fillStates(State **states, Pattern **patterns) {
    for (int i = 0; i < length(patterns); i++) {
        Pattern *p = patterns[i];
        int row = p->base;
        State *s = states[row];
        int n = length(s->patterns);
        s->patterns = adjust(s->patterns, +1);
        s->patterns[n] = p;
    }
}

// Check whether patterns p and q can be treated as successive range elements.
bool compatible(Pattern *p, Pattern *q) {
    if (p->look != q->look) return false;
    if (strlen(p->string) != 1 || strlen(q->string) != 1) return false;
    if (p->string[0] + 1 != q->string[0]) return false;
    if (! equal(p->targetName, q->targetName)) return false;
    if (! equal(p->typeName, q->typeName)) return false;
    return true;
}

// Print a state's patterns. After range expansion and sorting, a state may have
// runs of many one-character patterns. Gather a run and print it as a range.
void printState(State *state) {
    Pattern **ps = state->patterns;
    if (state->start || state->after) {
        printf("%s: (", state->name);
        if (state->start) printf("start");
        if (state->start && state->after) printf(", ");
        if (state->after) printf("after");
        printf(")\n");
    }
    for (int i = 0; i < length(ps); i++) {
        int j = i;
        while (j < length(ps)-1 && compatible(ps[j], ps[j+1])) j++;
        if (j == i) printPattern(ps[i]);
        else {
            Pattern range = *ps[i];
            char string[7];
            sprintf(string, "%c .. %c", ps[i]->string[0], ps[j]->string[0]);
            range.string = string;
            printPattern(&range);
            i = j;
        }
    }
    printf("\n");
}

// Stage 3: create states, correlate them with patterns, optionally print.
State **getStates(Pattern **patterns, bool print) {
    State **states = makeStates(patterns);
    finishPatterns(patterns, states);
    fillStates(states, patterns);
    if (print) for (int i=0; i < length(states); i++) printState(states[i]);
    return states;
}

// ---------- Ranges -----------------------------------------------------------
// Expand ranges such as 0..9 to multiple one-character patterns, with more
// specific patterns (subranges and individual characters) taking precedence.
// Then sort the patterns in each state.

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

// Stage 4: expand ranges. Sort. Optionally print.
void expandRanges(State **states, bool print) {
    derangeAll(states);
    sortAll(states);
    if (print) for (int i=0; i < length(states); i++) printState(states[i]);
}

// ---------- Checks -----------------------------------------------------------
// Check that a scanner handles every input unambiguously. Check whether states
// occur at the start of tokens, or after the start, or both. Check that if
// after, \s \n patterns have types. Check that the scanner doesn't get stuck in
// an infinite loop.

// Set the start and after flags deduced from a state's patterns. Return true
// if any changes were caused.
bool deduce(State *state, State **states) {
    bool changed = false;
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern *p = state->patterns[i];
        State *target = states[p->target];
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
            changed = changed || deduce(states[i], states);
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
            if (popType(p->type) && popType(q->type) && p->type != q->type) {
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

// Check that bracket types are not associated with lookaheads.
void checkBrackets(State *base) {
    for (int i = 0; i < length(base->patterns); i++) {
        Pattern *p = base->patterns[i];
        if (! p->look) continue;
        if (! pushType(p->type) && ! popType(p->type)) continue;
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
        State *target = states[state->patterns[i]->target];
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

// Stage 4: carry out checks. Optionally print.
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

// ---------- Transforms -------------------------------------------------------
// Transform the rules before compiling. For each state s which has both the
// start and after flags set, convert it into two states, s and s' where s
// only occurs at the start of tokens, and s' occurs only after the start. This
// helps to solve several problems, and avoid having to make special cases of
// them in the scanner itself.

// Where a state has a pattern involving a close bracket, and it is not followed
// by another close bracket pattern for the same string, add a Miss pattern.
// For example,
//    s } t BlockE
//    s } u GroupE
//    s } s Miss
void addMiss(State *s) {
    for (int i = 0; i < length(s->patterns); i++) {
        Pattern *p = s->patterns[i];
        if (! popType(p->type)) continue;
        bool last = false;
        if (i == length(s->patterns) - 1) last = true;
        else if (! equal(p->string, s->patterns[i+1]->string)) last = true;
        else if (! popType(s->patterns[i+1]->type)) last = true;
        if (! last) continue;
        adjust(s->patterns, +1);
        for (int j = length(s->patterns)-2; j > i; j--) {
            s->patterns[j+1] = s->patterns[j];
        }
        s->patterns[i+1] = newObject(sizeof(Pattern));
        *s->patterns[i+1] = *s->patterns[i];
        s->patterns[i+1]->typeName = "Miss";
        s->patterns[i+1]->type = Miss;
        s->patterns[i+1]->targetName = s->name;
        s->patterns[i+1]->target = s->row;
        i++;
    }
}

// If a state has both start and after set, create a new partner state with the
// same name. If the name is s, the pair can be printed as s and s'. Copy the
// patterns to the partner state, set one flag in each, and set the partner
// fields to refer to each other.
void splitState(State **states, int s) {
    State *state = states[s];
    if (! (state->start && state->after)) return;
    int partner = length(states);
    char *newName = newObject(strlen(state->name) + 2);
    sprintf(newName, "%s'", state->name);
    states = addState(states, newName);
    State *other = states[partner];
    for (int i = 0; i < length(state->patterns); i++) {
        other->patterns = adjust(other->patterns, +1);
        other->patterns[i] = newObject(sizeof(Pattern));
        *other->patterns[i] = *state->patterns[i];
    }
    state->after = false;
    other->after = true;
    state->partner = states[partner];
    other->partner = states[s];
}

// Set the target t of every pattern in a state to t or t', as appropriate. For
// a \s or \n rule in an s', change the target to s so that s' deals with the
// final token before the separator, and s deals with the separator.
void retarget(State **states, int i) {
    State *s = states[i];
    for (int i = 0; i < length(s->patterns); i++) {
        Pattern *p = s->patterns[i];
        State *t = states[p->target];
        if (s->partner != NULL && s->after) {
            if (p->string[0] == ' ' || p->string[0] == '\n') {
                p->target = s->partner->row;
                p->targetName = s->partner->name;
                continue;
            }
        }
        bool change = false;
        if (p->type != None && ! t->start) change = true;
        if (p->type == None && ! p->look && ! t->after) change = true;
        if (p->type == None && p->look && t->start != s->start) change = true;
        if (change) {
            p->target = t->partner->row;
            p->targetName = t->partner->name;
        }
    }
}

// Stage 5: Split the states as necessary. Change the targets. Add Miss patterns
// for close brackets. Carry out all the transformations on old and new
// states. Optionally print.
void transformAll(State **states, bool print) {
    int n = length(states);
    for (int i = 0; i < n; i++) {
        addMiss(states[i]);
    }
    for (int i = 0; i < n; i++) splitState(states, i);
    n = length(states);
    for (int i = 0; i < n; i++) {
//        retarget(states, i);
//        transform(states[i]);
    }
    if (print) for (int i=0; i < length(states); i++) printState(states[i]);
}

int main() {
    char **lines = getLines("c.txt");
    Pattern **patterns = getPatterns(lines, false);
    State **states = getStates(patterns, false);
    expandRanges(states, false);
    checkAll(states, false);
    transformAll(states, true);
    freeAll();
}


/*


// In a state with start flag set, convert \s and \n patterns into non-lookahead
// SP, NL patterns. Change the corresponding type on \n from Quote to Miss, or
// Note to NoteE, and everything else to Gap or Newline. That ensures that
// unclosed quotes get treated as mismatched, and unclosed one-line comments
// get treated as matched, and no attempts are made to create empty tokens.
void transform(State *s) {
    if (! s->start) return;
    for (int i = 0; i < length(s->patterns); i++) {
        Pattern *p = s->patterns[i];
        if (p->string[0] != ' ' && p->string[0] != '\n') continue;
        p->look = false;
        if (p->string[0] == ' ') p->type = Gap;
        else if (p->type == Quote) p->type = NoteE;
        else if (p->type == Note) p->type = NoteE;
        else p->type = Newline;
    }
}


// ---------- Compiling --------------------------------------------------------
// Compile the states into a compact transition table. The table has a row for
// each state, and an overflow area used when there is more than one pattern
// for a particular character. Each row consists of 96 entries of two bytes
// each, one for each character \s, !, ..., ~, \n. The scanner uses the current
// state and the next character in the source text to look up an entry. The
// entry may be an action for that single character, or an offset relative to
// the start of the table to a list of patterns in the overflow area starting
// with that character, with their actions.

typedef unsigned char byte;

// Flags added to the type in an action. The LINK flag indicates that the action
// is a link to the overflow area. The LOOK flag indicates a lookahead pattern.
enum { LINK = 0x80, LOOK = 0x40 };

// Fill in an action for a simple pattern: [LOOK+type, target]
void compileAction(byte *action, Pattern *p) {
    int type = p->type;
    int target = p->target;
    bool look = p->look;
    if (look) type = LOOK | type;
    action[0] = type;
    action[1] = target;
}

// When there is more than one pattern for a state starting with a character,
// enter [LINK+hi, lo] where [hi,lo] is the offset to the overflow area.
void compileLink(byte *action, int offset) {
    action[0] = LINK | ((offset >> 8) & 0x7F);
    action[1] = offset & 0xFF;
}

// Fill in a pattern at the end of the overflow area. The pattern is stored as a
// byte containing the length, followed by the characters of the pattern after
// the first, followed by the action. For example <= with type OP and target t
// is stored as 4 bytes [2, '=', OP, t].
void compileExtra(Pattern *p, byte *table) {
    int len = strlen(p->string);
    byte *entry = &table[length(table)];
    entry[0] = len;
    strncpy((char *)&entry[1], p->string + 1, len - 1);
    compileAction(&entry[len], p);
}

int main() {
    char *text = readFile("c.txt");
    normalize(text);
    char **lines = splitLines(text);
    Rule **rules = getRules(lines);
    State **states = makeStates(rules);
    fillStates(rules, states);
    derangeAll(states);
    sortAll(states);
    checkAll(states);
    transformAll(states);

//    printState(states[0], states);
    printState(states[18], states);
    printf("\n");
    printState(states[26], states);

    freeAll();
}
*/

/*
//==============================================================================

// Fill in a stack op. For example, "s body t T -body" (in HTML) becomes
// [POP, body, 2, 0, T, t]. If the input is "b" but not "body" the [2, 0] is a
// guaranteed mismatch to skip [T, t]. The POP action jumps to [T,t] if
// successful, or past it if not. After a POP or a sequence of POPs for the
// same pattern string, add a similar Miss op.
void compileOp(byte *b, Pattern *p) {

}
*/
/*
//==============================================================================
// TODO: convert +t -t to PUSH, POP and add a Miss pattern.




// Compile a state's patterns into the given table row, where end is the
// current end of the table overflow. Return the new end.
int compileState(State *base, int row, byte *table, int end) {
    Pattern **patterns = base->patterns;
    char prev = '\0';
    for (int i = 0; i < length(patterns); i++) {
        Pattern *p = patterns[i];
        char ch = p->match[0];
        int col = (ch == '\n') ? 95 : ch - ' ';
        byte *entry = &table[2 * (96 * row + col)];
        if (ch != prev) {
            prev = ch;
            if (strlen(p->match) == 1) {
                compileAction(entry, p);
                continue;
            }
            compileLink(entry, end);
        }
        end = compileExtra(p, table, end);
    }
    return end;
}

// Fill in the patterns from the position in the given array which start with
// the same character. If there is one pattern (necessarily a singleton
// character), put an action in the table. Otherwise, put a link in the table
// and put the patterns and actions in the overflow area. The group of patterns
// doesn't need to be terminated because the last pattern is always a singleton
// character which matches. Return the new index in the array of patterns.
// int compileGroup(Pattern **patterns, int n, byte *table, int row) {
//     bool immediate = strlen(patterns[n]->match) == 1;
//     char ch = patterns[n]->match[0];
//     int col = (ch == '\n') ? 95 : ch - ' ';
//     byte *entry = &table[2 * (96 * row + col)];
//     if (immediate) {
//         compileAction(entry, patterns[n]);
//         return n+1;
//     }
//     compileLink(entry, size(table));
//     while (n < length(patterns) && patterns[n]->match[0] == ch) {
//         compileExtra(patterns[n], table);
//         n++;
//     }
//     return n;
// }

// Fill all the patterns from all states into the table, where end is the current
// end of the table's overflow area.
void compile(State **states, byte *table, int end) {
    for (int i = 0; i < length(states); i++) {
        State *base = states[i];
        end = compileState(base, i, table, end);

        Pattern **patterns = base->patterns;
        int n = 0;
        while (n < length(patterns)) {
            n = compileGroup(patterns, n, table, i, ov);
        }

    }
}

// ---------- Scanning ---------------------------------------------------------
// A line of source text is scanned to produce an array of bytes, one for each
// character. The first byte corresponding to a token gives its type (e.g. ID
// for an identifier). The bytes corresponding to the remaining characters of
// the token contain None. The scanner first marks any leading spaces, then uses
// the transition table to handle the remaining characters.

// TODO: change logic:
// while (! match) {
//   act = 0x80 ? LINK : type&0x3F
//   match = true
//   switch (act) {
//      None: do nothing
//      Gap: if not at start, match = false else tokenise
//      Newline: similar
//      POP: if brk = any, mismatch else if not eq match = false else pop
//          if match +2 and continue else +4
//      PUSH: push
//      default: terminate token
//   }
//   if (!match) { get len check pattern }
// }

// Use the given table and start state to scan the given input line, producing
// the result in the given byte array, and returning the final state. Use
// the states for names when tracing.
int scan(byte *table, int st, char *in, byte *out, State **states, bool trace) {
    int n = strlen(in);
    for (int i = 0; i<=n; i++) out[i] = None;
    int at = 0, start = 0;
    while (at <= n) {
        char ch = in[at];
        if (trace) printf("%s ", states[st]->name);
        int col = ch - ' ';
        if (ch == '\0') col = 95;
        byte *action = &table[2 * (96 * st + col)];
        int len = 1;
        if ((action[0] & LINK) != 0) {
            int offset = ((action[0] & 0x7F) << 8) + action[1];
            byte *p = table + offset;
            bool match = false;
            while (! match) {
                match = true;
                len = p[0];
                for (int i = 1; i < len && match; i++) {
                    if (in[at + i] != p[i]) match = false;
                }
                if (match) break;
                else p = p + len + 2;
            }
            action = p + len;
        }
        bool lookahead = (action[0] & LOOK) != 0;
        int tag = action[0] & 0x3F;
        int target = action[1];
        if (trace) {
            if (lookahead) printf("\\ ");
            if (in[at] == ' ') printf("SP");
            else if (in[at] == '\0') printf("NL");
            else for (int i = 0; i < len; i++) printf("%c", in[at+i]);
            printf(" %s\n", tagNames[tag]);
        }
        if (! lookahead) at = at + len;
        if (tag != None && start < at) {
            out[start] = tag;
            start = at;
        }
        if (ch == ' ') { out[at++] = Gap; start++; }
        if (ch == '\0') { out[at++] = Newline; start++; }
        st = target;
    }
    return st;
}

// ---------- Testing ----------------------------------------------------------
// The tests in a language description are intended to check that the rules work
// as expected. They also act as tests for this program. A line starting with >
// is a test and one starting with < is the expected output.

// Carry out a test, given a line of input and an expected line of output.
int runTest(
    byte *table, int st, char *in, char *expected, State **states, bool trace
) {
    int n = strlen(in) + 1;
    byte bytes[n+1];
    st = scan(table, st, in+2, bytes+2, states, trace);
    char out[100] = "< ";
    for (int i = 2; i < n; i++) out[i] = tagNames[bytes[i]][0];
    out[n] = '\0';
    if (strcmp(out, expected) == 0) return st;
    printf("Test failed. Input, expected output and actual output are:\n");
    printf("%s\n", in);
    printf("%s\n", expected);
    printf("%s\n", out);
    exit(1);
    return st;
}

void runTests(byte *table, char **lines, State **states, bool trace) {
    int st = 0;
    int count = 0;
    for (int i = 0; i < length(lines); i++) {
        if (lines[i][0] != '>') continue;
        char *in = lines[i];
        char *expected = lines[i+1];
        st = runTest(table, st, in, expected, states, trace);
        count++;
    }
    printf("Passed %d tests.\n", count);
}

// ---------- Writing ----------------------------------------------------------
// The table and its overflow are written out to a binary file.

void write(char *path, byte *table) {
    FILE *p = fopen(path, "wb");
    fwrite(table, size(table), 1, p);
    fclose(p);
}

int main(int n, char *args[n]) {
    bool trace = (n < 2) ? false : strcmp(args[1], "-t") == 0;
    char *file = args[n-1];
    bool txt = strcmp(file + strlen(file) - 4, ".txt") == 0;
    if (n > 3 || (n == 3 && ! trace) || ! txt) {
        printf("Usage: compile [-t] c.txt\n");
        exit(0);
    }
    char *text = readFile(file);
    normalize(text);
    char **lines = splitLines(text);
    Rule **rules = getRules(lines);
    State **states = makeStates(rules);
    fillStates(rules, states);
    derangeAll(states);
    sortAll(states);
    checkAll(states);

// int count = 0;
// for (int i = 0; i < size(states); i++) {
//     printf("%s: %d %d\n", states[i].name, states[i].start, states[i].after);
//     if (states[i].start) count++;
// }
// printf("#states %d\n", size(states));
// printf("%d more needed for starters\n", count);

    byte *table = malloc(2*96*length(states) + 10000);
    compile(states, table, 2*96*length(states));
    runTests(table, lines, states, trace);

    char outfile[100];
    strcpy(outfile, file);
    strcpy(outfile + strlen(outfile) - 4, ".bin");
    write(outfile, table);

    free(table);
    for (int i = 0; i < length(states); i++) freeArray(states[i]->patterns);
    freeArray(states);
    for (int i = 0; i < length(rules); i++) freeArray(rules[i]->strings);
    freeArray(rules);
    freeArray(lines);
    free(text);
}


*/
