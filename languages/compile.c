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
// TODO: ensure().  BUT f(a){a=ensure(a);return a;}

// ---------- types ------------------------------------------------------------
// These types and their names must be kept the same as in other Snipe modules.
// A type is used to mark a text character, to represent the result of
// scanning. The bracket types come in matching pairs, with a B or E suffix.
// A few types and flags are used internally:
//   NONE (no explicit type) marks token characters after the first.
//   GAP and NEWLINE mark a space or newline as a separator.
//   MISS is used as a close bracket which forces a mismatch.
//   COMMENTED and BAD are flags which can be added and removed.
// X+COMMENTED -> NOTE, and X+BAD -> WRONG, for display.

enum type {
    NONE, GAP, NEWLINE, MISS, ALTERNATIVE, DECLARATION, FUNCTION,
    IDENTIFIER, JOIN, KEYWORD, LONG, MARK, NOTE, OPERATOR, PROPERTY, QUOTE,
    TAG, UNARY, VALUE, WRONG,

    QUOTEB, LONGB, NOTEB, COMMENTB, COMMENTNB, TAGB, ROUNDB, ROUND2B,
    SQUAREB, SQUARE2B, GROUPB, GROUP2B, BLOCKB, BLOCK2B,

    QUOTEE, LONGE, NOTEE, COMMENTE, COMMENTNE, TagE, ROUNDE, ROUND2E,
    SQUAREE, SQUARE2E, GROUPE, GROUP2E, BLOCKE, BLOCK2E,

    COMMENTED = 64, BAD = 128,
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
    return QUOTEB <= type && type <= BLOCK2B;
}

bool popType(int type) {
    return QUOTEE <= type && type <= BLOCK2E;
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
int findtype(char *s, int row) {
    if (s == NULL) return NONE;
    for (int i = ALTERNATIVE; i <= BLOCK2E; i++) {
        char *name = typeNames[i];
        if (equal(s, name)) return i;
        if (! islower(name[strlen(name)-1])) continue;
        if (prefix(s, name)) return i;
    }
    error("unknown type %s on line %d", s, row);
    return -1;
}

// ---------- Arrays -----------------------------------------------------------
// An array is preceded by a (pointer-aligned) header with length. Its capacity
// is at least the maximum ever needed, to avoid reallocation. For an array of
// pointers to structures, the pointers and structures are allocated at the
// same time, and the pointers are initialized. All allocations are chained
// onto a global chain for easy freeing.

typedef struct header header;
struct header { int length; header *next; };

header *allArrays = NULL;

void *array(int max, int unit) {
    header *p = malloc(sizeof(header) + max * unit);
    p->length = 0;
    p->next = allArrays;
    allArrays = p;
    return p + 1;
}

void *arrayP(int max, int unit) {
    void **pp = array(max, sizeof(void *) + unit);
    char *ps = (char *) (pp + max);
    for (int i = 0; i < max; i++) pp[i] = &ps[i * unit];
    return pp;
}

int length(void *a) {
    header *p = (header *) a - 1;
    return p->length;
}

// Adjust length from n to n+d. Return n.
int adjust(void *a, int d) {
    header *p = (header *) a - 1;
    p->length += d;
    return p->length - d;
}

void freeAll() {
    while (allArrays != NULL) {
        header *block = allArrays;
        allArrays = allArrays->next;
        free(block);
    }
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
    char *text =  array(size + 2, 1);
    fread(text, 1, size, file);
    if (text[size-1] != '\n') text[size++] = '\n';
    text[size] = '\0';
    adjust(text, size);
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
    int n = 0, start = 0;
    for (int i = 0; text[i] != '\0'; i++) if (text[i] == '\n') n++;
    char **lines = array(n, sizeof(char *));
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i] = '\0';
        char *line = trim(&text[start]);
        lines[adjust(lines,+1)] = line;
        start = i + 1;
    }
    return lines;
}

// ---------- Rules ------------------------------------------------------------
// Extract the rules from the text, as strings.

// A rule is a line number and strings, including an array of pattern strings.
struct rule { int row; char *base, *target, *type; char **strings; };
typedef struct rule Rule;

// Split a line into strings in place.
char **splitStrings(char *line) {
    int len = strlen(line), start = 0;
    char **strings = array(len, sizeof(char *));
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] != ' ') continue;
        line[i] = '\0';
        if (i > 0 && line[i-1] != '\0') {
            strings[adjust(strings,+1)] = &line[start];
        }
        start = i + 1;
    }
    strings[adjust(strings,+1)] = &line[start];
    return strings;
}

// Fill in a rule from an array of strings.
void fillRule(Rule *rule, int row, char **strings) {
    rule->row = row;
    if (length(strings) < 3) error("incomplete rule on line %d", row);
    rule->base = strings[0];
    for (int i = 1; i < length(strings); i++) strings[i-1] = strings[i];
    adjust(strings, -1);
    char *s = strings[length(strings) - 1];
    if (isupper(s[0])) {
        rule->type = s;
        adjust(strings, -1);
        if (length(strings) < 2) error("incomplete rule on line %d", row);
    }
    else rule->type = NULL;
    s = strings[length(strings) - 1];
    if (! islower(s[0])) error("expecting target state on line %d", row);
    rule->target = s;
    adjust(strings, -1);
    rule->strings = strings;
}

// Extract the rules from the lines.
Rule **getRules(char **lines) {
    Rule **rules = arrayP(length(lines), sizeof(Rule));
    for (int i = 0; i < length(lines); i++) {
        if (! islower(lines[i][0])) continue;
        int n = adjust(rules, +1);
        fillRule(rules[n], i+1, splitStrings(lines[i]));
    }
    return rules;
}

// Count up the patterns belonging to a named state. Add 96 for possible
// additional one-character patterns when ranges are expanded. Double for
// possible added patterns.
int countPatterns(Rule **rules, char *name) {
    int n = 96;
    for (int i = 0; i < length(rules); i++) {
        if (! equal(name, rules[i]->base)) continue;
        n = n + length(rules[i]->strings);
    }
    return 2 * n;
}

// ---------- States and patterns ----------------------------------------------
// Convert the rules into an array of states with patterns.

// A pattern is a string to be matched, together with the type and target.
struct pattern { char *string; bool look; int row, type, target; };
typedef struct pattern Pattern;

// A state has a name, and an array of patterns. It has flags to say whether it
// occurs at or after the start of tokens. It has a visited flag used when
// checking for infinite loops. It has a partner state if it is one of a pair
// s', s" resulting from splitting s with both start and after flags set.
struct state {
    char *name; Pattern **patterns;
    bool start, after, visited;
    int partner;
};
typedef struct state State;

// Single character strings covering \n \s !..~ for expanding ranges etc.
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

// Find an existing state by name, returning its index or -1.
int findState(State **states, char *name) {
    for (int i = 0; i < length(states); i++) {
        if (equal(states[i]->name, name)) return i;
    }
    return -1;
}

// Add a new empty state with the given name.
void addState(State **states, char *name, int maxPatterns) {
    int n = adjust(states, +1);
    Pattern **ps = arrayP(maxPatterns, sizeof(Pattern));
    *states[n] = (State) {
        .name = name, .patterns = ps,
        .start = false, .after = false, .visited = false,
        .partner = -1
    };
}

// Create empty base states from the rules. Allow for later state splitting.
State **makeStates(Rule **rules) {
    State **states = arrayP(2 * length(rules), sizeof(State));
    for (int i = 0; i < length(rules); i++) {
        Rule *rule = rules[i];
        if (findState(states, rule->base) < 0) {
            addState(states, rule->base, countPatterns(rules, rule->base));
        }
    }
    return states;
}

// Fill a pattern from a string, type and target. Normalise lookaheads:
//    \\\... -> look \...
//    \\...  -> \...
//    \x     -> error except \s \n
//    \...   -> look ...
//    \      -> look NL..~
void fillPattern(Pattern *p, char *s, int row, int type, int target) {
    p->row = row;
    p->type = type;
    p->target = target;
    p->look = false;
    if (prefix("\\\\\\", s)) {
        p->look = true;
        s = &s[2];
    }
    else if (prefix("\\\\", s)) {
        s = &s[1];
    }
    else if (prefix("\\", s) && islower(s[1]) && s[2] == '\0') {
        if (s[1] != 's' && s[1] != 'n') {
            error("bad lookahead %s on line %d", s, row);
        }
        p->look = true;
        if (s[1] == 's') s = singles[' '];
        if (s[1] == 'n') s = singles['\n'];
    }
    else if (prefix("\\", s) && s[1] != '\0') {
        p->look = true;
        s = &s[1];
    }
    else if (prefix("\\", s)) {
        p->look = true;
        s = "\n..~";
    }
    p->string = s;
}

// Transfer pattern strings from a rule into its base state.
void fillState(Rule *r, State **states) {
    int index = findState(states, r->base);
    State *base = states[index];
    int target = findState(states, r->target);
    int type = findtype(r->type, r->row);
    if (target < 0) {
        error("unknown target state %s on line %d", r->target, r->row);
    }
    char **strings = r->strings;
    int n = length(strings);
    for (int i = 0; i < n; i++) {
        char *s = strings[i];
        int m = adjust(base->patterns, +1);
        Pattern *p = base->patterns[m];
        fillPattern(p, s, r->row, type, target);
    }
}

// Transfer patterns from the rules to the states.
void fillStates(Rule **rules, State **states) {
    for (int i = 0; i < length(rules); i++) {
        fillState(rules[i], states);
    }
}

// Print a state name. When s has been split into s and s', add the suffix.
void printName(State *s) {
    char *suffix = "";
    if (s->partner >= 0 && s->after) suffix = "'";
    printf("%s%s", s->name, suffix);
}

// Print a pattern string with a space on either side. Convert back to the
// original format, with backslash for lookahead, and double backslash for
// explicit backslash. For a non-lookahead pattern for space or newline, which
// isn't legal on the original format, just print SP or NL.
void printPattern(Pattern *p) {
    printf(" ");
    if (p->look) printf("\\");
    if (p->string[0] == '\\') printf("\\");
    if (! p->look && p->string[0] == '\n') printf("NL");
    else if (p->string[0] == '\n') printf("n");
    else if (! p->look && p->string[0] == ' ') printf("SP");
    else if (p->string[0] == ' ') printf("s");
    else printf("%s", p->string);
    printf(" ");
}

// Print out a state as lots of one-pattern rules. Compress one-character
// patterns into ranges where possible.
void printState(State *state, State **states) {
    char range[5] = "?..?";
    bool combined = false;
    for (int i = 0; i < length(state->patterns); i++) {
        Pattern **ps = state->patterns;
        Pattern *p = state->patterns[i];
        bool combineWithNext = true;
        if (i == length(state->patterns) - 1) combineWithNext = false;
        else if (strlen(p->string) != 1) combineWithNext = false;
        else if (p->string[0] == ' ') combineWithNext = false;
        else if (p->string[0] == '\n') combineWithNext = false;
        else if (strlen(ps[i+1]->string) != 1) combineWithNext = false;
        else if (ps[i+1]->string[0] == ' ') combineWithNext = false;
        else if (ps[i+1]->string[0] == '\n') combineWithNext = false;
        else if (p->look != ps[i+1]->look) combineWithNext = false;
        else if (p->target != ps[i+1]->target) combineWithNext = false;
        else if (p->type != ps[i+1]->type) combineWithNext = false;
        if (combineWithNext) {
            if (! combined) range[0] = p->string[0];
            combined = true;
            continue;
        }
        printName(state);
        if (combined) {
            range[3] = p->string[0];
            printf(" %s ", range);
            combined = false;
        }
        else printPattern(p);
        printName(states[p->target]);
        if (p->type != NONE) {
            if (p->type == GAP) printf(" Gap");
            else if (p->type == NEWLINE) printf(" Newline");
            else if (p->type == MISS) printf(" Miss");
            else printf(" %s", typeNames[p->type]);
        }
        printf("\n");
    }
}

// ---------- Ranges -----------------------------------------------------------
// Expand ranges such as 0..9 to several one-character patterns, with more
// specific patterns (subranges and individual characters) taking precedence.
// A range may be NL..~ to represent the \ abbreviation.

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

// Remove the i'th pattern from a state's patterns. The patterns are not yet
// sorted, so just replace it with the last pattern.
void delete(State *base, int i) {
    int n = length(base->patterns);
    *base->patterns[i] = *base->patterns[n-1];
    adjust(base->patterns, -1);
}

// Add a singleton pattern if not already handled.
void addSingle(State *base, Pattern *range, int ch) {
    bool found = false;
    int n = length(base->patterns);
    for (int i = 0; i < n; i++) {
        char *s = base->patterns[i]->string;
        if (s[0] == ch && s[1] == '\0') { found = true; break; }
    }
    if (found) return;
    adjust(base->patterns, +1);
    *base->patterns[n] = *range;
    base->patterns[n]->string = singles[ch];
}

// Expand a state's range into singles, and add them if not already handled.
void derange(State *base, Pattern *range) {
    char *s = range->string;
    for (int ch = s[0]; ch <= s[3]; ch++) {
        if (ch == '\n' || ch >= ' ') addSingle(base, range, ch);
    }
}

// For a given state, find a most specific range, expand it, return success.
bool derangeState(State *base) {
    int index = -1;
    for (int i = 0; i < length(base->patterns); i++) {
        char *s = base->patterns[i]->string;
        if (! isRange(s)) continue;
        if (index >= 0) {
            char *t = base->patterns[index]->string;
            if (overlap(s,t)) {
                error("ranges %s %s overlap in %s", s, t, base->name);
            }
            if (subRange(s,t)) index = i;
        }
        else index = i;
    }
    bool success = (index >= 0);
    if (success) {
        Pattern *range = base->patterns[index];
        derange(base, range);
        delete(base, index);
    }
    return success;
}

// Expand all ranges in all states.
void derangeAll(State **states) {
    for (int i = 0; i < length(states); i++) {
        bool found = true;
        while (found) found = derangeState(states[i]);
    }
}

// ---------- Sorting ----------------------------------------------------------
// Sort the patterns for each state into lexicographic order, except that if s
// is a prefix of t, t comes before s.

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
        State *base = states[i];
        sort(base->patterns);
    }
}

// ---------- Checks -----------------------------------------------------------
// Check that a scanner handles every input unambiguously. Check whether states
// occur at the start of tokens, or after the start, or both. Check that if
// after, \s \n patterns have types. Check that the scanner doesn't get stuck in
// an infinite loop.

// Check that a state has no duplicate patterns, except for stack pops.
void noDuplicates(State *base) {
    Pattern **list = base->patterns;
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
            error("state %s has pattern for %s twice", base->name, s);
        }
    }
}

// Check that a state handles every singleton character.
void complete(State *base) {
    char ch = '\n';
    for (int i = 0; i < length(base->patterns); i++) {
        char *s = base->patterns[i]->string;
        if (s[1] != '\0') continue;
        if (s[0] == ch) {
            if (ch == '\n') ch = ' ';
            else ch = ch + 1;
        }
    }
    if (ch > '~') return;
    if (ch == ' ') error("state %s doesn't handle \\s", base->name);
    else if (ch == '\n') error("state %s doesn't handle \\n", base->name);
    else error("state %s doesn't handle %c", base->name, ch);
}

// Check that bracket types are not associated with lookaheads.
void checkBrackets(State *base) {
    for (int i = 0; i < length(base->patterns); i++) {
        Pattern *p = base->patterns[i];
        if (! p->look) continue;
        if (! pushType(p->type) && ! popType(p->type)) continue;
        error("bracket type with lookahead on line %d", p->row);
    }
}

// Set the start and after flags deduced from a state's patterns. Return true
// if any changes were caused.
bool deduce(State *base, State **states) {
    bool changed = false;
    for (int i = 0; i < length(base->patterns); i++) {
        Pattern *p = base->patterns[i];
        State *target = states[p->target];
        if (p->type != NONE) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->type == NONE && ! p->look) {
            if (! target->after) changed = true;
            target->after = true;
        }
        if (p->type == NONE && p->look && base->start) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->type == NONE && p->look && base->after) {
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

// Check, for a state with the after flag, that \s \n patterns have types.
void separates(State *base) {
    if (! base->after) return;
    for (int i = 0; i < length(base->patterns); i++) {
        Pattern *p = base->patterns[i];
        if (p->string[0] == ' ' || p->string[0] == '\n') {
            if (p->type == NONE) {
                error(
                    "state %s should terminate tokens on matching \\s or \\n",
                    base->name
                );
            }
        }
    }
}

// Search for a chain of lookaheads from a given state, other than \s or \n,
// which can cause an infinite loop. The look argument is the longest lookahead
// in the chain so far.
void follow(State **states, State *base, char *look) {
    if (base->visited) error("state %s can loop", base->name);
    base->visited = true;
    for (int i = 0; i < length(base->patterns); i++) {
        if (! base->patterns[i]->look) continue;
        char *s = base->patterns[i]->string;
        if (s[0] == ' ' || s[0] == '\n') continue;
        if (s[0] > look[0]) break;
        if (s[0] < look[0]) continue;
        char *next;
        if (prefix(s, look)) next = look;
        else if (prefix(look, s)) next = s;
        else continue;
        State *target = states[base->patterns[i]->target];
        follow(states, target, next);
    }
    base->visited = false;
}

// Start a search from a given state, for each possible input character.
void search(State **states, State *base) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if ('\n' < ch && ch < ' ') continue;
        char *look = singles[ch];
        follow(states, base, look);
    }
}

void checkAll(State **states) {
    for (int i = 0; i < length(states); i++) {
        noDuplicates(states[i]);
        complete(states[i]);
        checkBrackets(states[i]);
    }
    deduceAll(states);
    for (int i = 0; i < length(states); i++) {
         separates(states[i]);
         search(states, states[i]);
    }
}

// ---------- Transforms -------------------------------------------------------
// Transform the rules before compiling. For each state s which has both the
// start and after flags set, convert it into two states, s and s' where s
// only occurs at the start of tokens, and s' occurs only after the start. This
// helps to solve several problems, avoiding having to make special cases of
// any of them in the scanner itself.

// If a state has both start and after set, create a new partner state with the
// same name. If the name is s, the pair can be printed as s and s'. Copy the
// patterns to the partner state, set one flag in each, and set the partner
// fields to refer to each other.
void splitState(State **states, int s) {
    State *base = states[s];
    if (! base->start || ! base->after) return;
    int partner = length(states);
    addState(states, base->name, length(base->patterns));
    State *other = states[partner];
    adjust(other->patterns, length(base->patterns));
    for (int i = 0; i < length(base->patterns); i++) {
        *other->patterns[i] = *base->patterns[i];
    }
    base->after = false;
    other->after = true;
    base->partner = partner;
    other->partner = s;
}

// Set the target t of every pattern in a state to t or t', as appropriate. For
// a \s or \n rule in an s', change the target to s so that s' deals with the
// final token before the separator, and s deals with the separator.
void retarget(State **states, int i) {
    State *s = states[i];
    for (int i = 0; i < length(s->patterns); i++) {
        Pattern *p = s->patterns[i];
        State *t = states[p->target];
        if (s->partner >= 0 && s->after) {
            if (p->string[0] == ' ' || p->string[0] == '\n') {
                p->target = s->partner;
                continue;
            }
        }
        bool change = false;
        if (p->type != NONE && ! t->start) change = true;
        if (p->type == NONE && ! p->look && ! t->after) change = true;
        if (p->type == NONE && p->look && t->start != s->start) change = true;
        if (change) p->target = t->partner;
    }
}

// Where a state has a pattern involving a close bracket, and it is not followed
// by another close bracket pattern for the same string, add a MISS pattern
// which doesn't change state. For example,
//    s } t BlockE
//    s } u GroupE
//    s } s MISS
void addMiss(State *s) {
//    if (equal(s->name,"fred")) printf("before: %d\n", length(s->patterns));
    if (equal(s->name,"fred")) printf("p13: %s\n", typeNames[s->patterns[13]->type]);
    for (int i = 0; i < length(s->patterns); i++) {
        Pattern *p = s->patterns[i];
        if (! popType(p->type)) continue;
        bool last = false;
        if (i == length(s->patterns) - 1) last = true;
        else if (! equal(p->string, s->patterns[i+1]->string)) last = true;
        else if (! popType(s->patterns[i+1]->type)) last = true;
        if (! last) continue;
        if (equal(s->name,"fred")) printf("%d\n", i);
        adjust(s->patterns, +1);
        for (int j = length(s->patterns)-2; j > i; j--) {
            *s->patterns[j+1] = *s->patterns[j];
        }
        *s->patterns[i+1] = *s->patterns[i];
        s->patterns[i+1]->type = MISS;
//        s->patterns[i+1]->target = ?
        i++;
    }
//    if (equal(s->name,"fred")) printf("after: %d\n", length(s->patterns));
    if (equal(s->name,"fred")) printf("p13: %s\n", typeNames[s->patterns[13]->type]);
}

// In a state with start flag set, convert \s and \n patterns into non-lookahead
// SP, NL patterns. Change the corresponding type on \n from QUOTE to MISS, or
// NOTE to NOTEE, and everything else to GAP or NEWLINE. That ensures that
// unclosed quotes get treated as mismatched, and unclosed one-line comments
// get treated as matched, and no attempts are made to create empty tokens.
void transform(State *s) {
    if (! s->start) return;
    for (int i = 0; i < length(s->patterns); i++) {
        Pattern *p = s->patterns[i];
        if (p->string[0] != ' ' && p->string[0] != '\n') continue;
        p->look = false;
        if (p->string[0] == ' ') p->type = GAP;
        else if (p->type == QUOTE) p->type = NOTEE;
        else if (p->type == NOTE) p->type = NOTEE;
        else p->type = NEWLINE;
    }
}

// Split all the old states (but not any newly created ones). Then carry out
// all the transformations on old and new states.
void transformAll(State **states) {
    int n = length(states);
    for (int i = 0; i < n; i++) splitState(states, i);
    n = length(states);
    for (int i = 0; i < n; i++) {
        retarget(states, i);
        addMiss(states[i]);
        transform(states[i]);
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


/*
//==============================================================================

// Fill in a stack op. For example, "s body t T -body" (in HTML) becomes
// [POP, body, 2, 0, T, t]. If the input is "b" but not "body" the [2, 0] is a
// guaranteed mismatch to skip [T, t]. The POP action jumps to [T,t] if
// successful, or past it if not. After a POP or a sequence of POPs for the
// same pattern string, add a similar MISS op.
void compileOp(byte *b, Pattern *p) {

}
*/
/*
//==============================================================================
// TODO: convert +t -t to PUSH, POP and add a MISS pattern.




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
// the token contain NONE. The scanner first marks any leading spaces, then uses
// the transition table to handle the remaining characters.

// TODO: change logic:
// while (! match) {
//   act = 0x80 ? LINK : type&0x3F
//   match = true
//   switch (act) {
//      NONE: do nothing
//      GAP: if not at start, match = false else tokenise
//      NEWLINE: similar
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
    for (int i = 0; i<=n; i++) out[i] = NONE;
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
        if (tag != NONE && start < at) {
            out[start] = tag;
            start = at;
        }
        if (ch == ' ') { out[at++] = GAP; start++; }
        if (ch == '\0') { out[at++] = NEWLINE; start++; }
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
