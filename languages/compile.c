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

// TODO arrays again.

// ---------- Arrays ------------------------------------------------------------
// An array is preceded by a (pointer-aligned) length. Its capacity is at least
// the maximum ever needed, to avoid reallocation. For an array of pointers to
// structures, the pointers and structures are allocated at the same time, and
// the pointers are initialized.

struct header { int length; void *align[]; };
typedef struct header header;

void *array(int max, int unit) {
    header *p = malloc(sizeof(header) + max * unit);
    p->length = 0;
    return p + 1;
}

void *arrayP(int max, int unit) {
    header *p = malloc(sizeof(header) + max * sizeof(void *) + max * unit);
    void **pp = (void **) p + 1;
    char *ps = (char *) (pp + max);
    p->length = 0;
    for (int i = 0; i < max; i++) pp[i] = &ps[i * unit];
    return p + 1;
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

void release(void *a) {
    free(((header *) a - 1));
}

// ---------- Tags -------------------------------------------------------------
// These tags and their names must be kept the same as in other Snipe modules. A
// tag is used to mark a text character, to represent the result of scanning.
// NONE is the default when there is no explicit tag, and marks token
// characters after the first. GAP and NEWLINE are used internally to mark a
// space or newline as a separator. For bracket tags, the version with no
// suffix is used when when there is no request to match a bracket, or after
// matching a bracket within a line. The versions with a suffix are for longer
// distance matching, with L for left, R for right where necessary. COMMENTED,
// DOCUMENTED and BAD are flags which can be added and removed. WRONG
// automatically has BAD added.

enum tag {
    NONE, GAP, NEWLINE, BEGIN, COMMENT, DOCUMENT, END, FUNCTION, IDENTIFIER,
    JOIN, KEYWORD, LEFT, MARK, NOTE, OPERATOR, PROPERTY, QUOTE, RIGHT, SIGN,
    TYPE, UNARY, VALUE, WRONG,

    QUOTE0L,    QUOTE1L,    QUOTE2L,    QUOTE3L,
    COMMENT0L,  COMMENT1L,  COMMENT2L,  COMMENT3L,
    DOCUMENT0L, DOCUMENT1L, DOCUMENT2L, DOCUMENT3L,
    BEGIN0,     BEGIN1,     BEGIN2,     BEGIN3,
    LEFT0,      LEFT1,      LEFT2,      LEFT3,

    QUOTE0R,    QUOTE1R,    QUOTE2R,    QUOTE3R,
    COMMENT0R,  COMMENT1R,  COMMENT2R,  COMMENT3R,
    DOCUMENT0R, DOCUMENT1R, DOCUMENT2R, DOCUMENT3R,
    END0,       END1,       END2,       END3,
    RIGHT0,     RIGHT1,     RIGHT2,     RIGHT3,

    COMMENTED=64, DOCUMENTED=128, BAD=64+128,
};

// The full names of the tags, with first character used in tests.
char *tagNames[64] = {
    " ", ".", "-", "BEGIN", "COMMENT", "DOCUMENT", "END", "FUNCTION",
    "IDENTIFIER", "JOIN", "KEYWORD", "LEFT", "MARK", "NOTE", "OPERATOR",
    "PROPERTY", "QUOTE", "RIGHT", "SIGN", "TYPE", "UNARY", "VALUE", "WRONG",

    "QUOTE0+", "QUOTE1+", "QUOTE2+", "QUOTE3+",
    "COMMENT0+", "COMMENT1+", "COMMENT2+", "COMMENT3+",
    "DOCUMENT0+", "DOCUMENT1+", "DOCUMENT2+", "DOCUMENT3+",
    "BEGIN0+", "BEGIN1+", "BEGIN2+", "BEGIN3+",
    "LEFT0+", "LEFT1+", "LEFT2+", "LEFT3+",

    "QUOTE0-", "QUOTE1-", "QUOTE2-", "QUOTE3-",
    "COMMENT0-", "COMMENT1-", "COMMENT2-", "COMMENT3-",
    "DOCUMENT0-", "DOCUMENT1-", "DOCUMENT2-", "DOCUMENT3-",
    "BEGIN0-", "BEGIN1-", "BEGIN2-", "BEGIN3-",
    "LEFT0-", "LEFT1-", "LEFT2-", "LEFT3-",
};

bool pushTag(int tag) {
    return QUOTE0L <= tag && tag <= LEFT3;
}

bool popTag(int tag) {
    return QUOTE0R <= tag && tag <= RIGHT3;
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

// Convert a base tag, with a variant and sign suffix, into an operation tag.
int opTag(int tag, int variant, int sign) {
    if (sign > 0) {
        switch (tag) {
        case BEGIN: tag = BEGIN0 + variant; break;
        case END: error("END should have - sign"); break;
        case LEFT: tag = LEFT0 + variant; break;
        case RIGHT: error("RIGHT should have - sign"); break;
        case QUOTE: tag = QUOTE0L + variant; break;
        case COMMENT: tag = COMMENT0L + variant; break;
        case DOCUMENT: tag = DOCUMENT0L + variant; break;
        default: error("tag %s should not have + or - sign", tagNames[tag]);
        }
    }
    if (sign < 0) {
        switch (tag) {
        case BEGIN: error("BEGIN should have + sign"); break;
        case END: tag = END0 + variant; break;
        case LEFT: error("LEFT should have + sign"); break;
        case RIGHT: tag = RIGHT0 + variant; break;
        case QUOTE: tag = QUOTE0R + variant; break;
        case COMMENT: tag = COMMENT0R + variant; break;
        case DOCUMENT: tag = DOCUMENT0R + variant; break;
        default: error("tag %s should not have + or - sign", tagNames[tag]);
        }
    }
    return tag;
}

// Convert a string to a tag. Handle suffixes and abbreviations and WRONG.
int findTag(char *s, int row) {
    if (s == NULL) return NONE;
    char name[16];
    strcpy(name, s);
    int sign = 0, variant = 0, tag = -1;
    int n = strlen(name);
    if ('0' <= name[n-1] && name[n-1] <= '3') {
        error("tag %s on line %d has digit suffix without sign", s, row);
    }
    if (name[n-1] == '+' || name[n-1] == '-') {
        if (name[n-1] == '+') sign = +1;
        else if (name[n-1] == '-') sign = -1;
        name[--n] = '\0';
        if ('0' <= name[n-1] && name[n-1] <= '3') {
            variant = name[n-1] - '0';
            name[--n] = '\0';
        }
    }
    for (int i = BEGIN; i <= WRONG; i++) {
        if (prefix(name, tagNames[i])) tag = i;
    }
    if (tag < 0) error ("unrecognised tag %s on line %d", s, row);
    tag = opTag(tag, variant, sign);
    if (tag == WRONG) tag += BAD;
    return tag;
}

// ---------- Lines ------------------------------------------------------------
// Read in a language description as a character array, normalize, and split the
// text into lines, in place.

// An array of strings, with length. Allocate max needed to avoid reallocation.
struct strings { int length; char *a[]; };
typedef struct strings Strings;

// Read file as string, ignore I/O errors, add final newline if necessary.
char *readFile(char *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *text =  malloc(size + 2);
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
Strings *splitLines(char *text) {
    int n = 0, start = 0;
    for (int i = 0; text[i] != '\0'; i++) if (text[i] == '\n') n++;
    Strings *lines = malloc(sizeof(Strings) + n * sizeof(char *));
    lines->length = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i] = '\0';
        char *line = trim(&text[start]);
        lines->a[lines->length++] = line;
        start = i + 1;
    }
    return lines;
}

// ---------- Rules ------------------------------------------------------------
// Extract the rules from the text, as strings.

// A rule is a line number and strings, including an array of pattern strings.
struct rule { int row; char *base, *target, *tag; Strings *strings; };
typedef struct rule Rule;

struct rules { int length; Rule a[]; };
typedef struct rules Rules;

// Split a line into strings in place.
Strings *splitStrings(char *line) {
    int len = strlen(line), start = 0;
    Strings *strings = malloc(sizeof(Strings) + len * sizeof(char *));
    strings->length = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] != ' ') continue;
        line[i] = '\0';
        if (i > 0 && line[i-1] != '\0') {
            strings->a[strings->length++] = &line[start];
        }
        start = i + 1;
    }
    strings->a[strings->length++] = &line[start];
    return strings;
}

// Fill in a rule from an array of strings.
void fillRule(Rule *rule, int row, Strings *strings) {
    rule->row = row;
    if (strings->length < 3) error("incomplete rule on line %d", row);
    rule->base = strings->a[0];
    for (int i = 1; i < strings->length; i++) strings->a[i-1] = strings->a[i];
    strings->length--;
    char *s = strings->a[strings->length - 1];
    if (isupper(s[0])) {
        rule->tag = s;
        strings->length--;
        if (strings->length < 2) error("incomplete rule on line %d", row);
    }
    else rule->tag = NULL;
    s = strings->a[strings->length - 1];
    if (! islower(s[0])) error("expecting target state on line %d", row);
    rule->target = s;
    strings->length--;
    rule->strings = strings;
}

// Extract the rules from the lines.
Rules *getRules(Strings *lines) {
    Rules *rules = malloc(sizeof(Rules) + lines->length * sizeof(Rule));
    rules->length = 0;
    for (int i = 0; i < lines->length; i++) {
        if (! islower(lines->a[i][0])) continue;
        int n = rules->length++;
        fillRule(&rules->a[n], i+1, splitStrings(lines->a[i]));
    }
    return rules;
}

// Count up the patterns belonging to a named state. Add 96 for possible
// additional one-character patterns when ranges are expanded.
int countPatterns(Rules *rules, char *name) {
    int n = 96;
    for (int i = 0; i < rules->length; i++) {
        if (strcmp(name, rules->a[i].base) != 0) continue;
        n = n + rules->a[i].strings->length;
    }
    return n;
}

// ---------- States and patterns ----------------------------------------------
// Convert the rules into an array of states with patterns.

// A pattern is a string to be matched, together with the tag and target.
struct pattern { char *string; bool look; int row, tag, target; };
typedef struct pattern Pattern;

struct patterns { int length; Pattern a[]; };
typedef struct patterns Patterns;

// A state has a name, and an array of patterns. It has flags to say whether it
// occurs at or after the start of tokens. It has a visited flag used when
// checking for infinite loops.
struct state { char *name; Patterns *patterns; bool start, after, visited; };
typedef struct state State;

struct states { int length; State a[]; };
typedef struct states States;

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
int findState(States *states, char *name) {
    for (int i = 0; i < states->length; i++) {
        if (strcmp(states->a[i].name, name) == 0) return i;
    }
    return -1;
}

// Add a new empty state with the given name.
void addState(States *states, char *name, int maxPatterns) {
    int n = states->length++;
    Patterns *ps = malloc(sizeof(Patterns) + maxPatterns * sizeof(Pattern));
    ps->length = 0;
    states->a[n] = (State) {
        .name=name, .patterns=ps,
        .start=false, .after=false, .visited=false
    };
}

// Create empty base states from the rules.
States *makeStates(Rules *rules) {
    States *states = malloc(sizeof(States) + rules->length * sizeof(State));
    states->length = 0;
    for (int i = 0; i < rules->length; i++) {
        Rule *rule = &rules->a[i];
        if (findState(states, rule->base) < 0) {
            addState(states, rule->base, countPatterns(rules, rule->base));
        }
    }
    return states;
}

// Fill a pattern from a string, tag and target. Normalise lookaheads:
//    \\\... -> look \...
//    \\...  -> \...
//    \x     -> error except \s \n
//    \...   -> look ...
//    \      -> look NL..~
void fillPattern(Pattern *p, char *s, int row, int tag, int target) {
    p->row = row;
    p->tag = tag;
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
void fillState(Rule *r, States *states) {
    int index = findState(states, r->base);
    State *base = &states->a[index];
    int target = findState(states, r->target);
    int tag = findTag(r->tag, r->row);
    if (target < 0) {
        error("unknown target state %s on line %d", r->target, r->row);
    }
    Strings *strings = r->strings;
    int n = strings->length;
    for (int i = 0; i < n; i++) {
        char *s = strings->a[i];
        int m = base->patterns->length++;
        Pattern *p = &base->patterns->a[m];
        fillPattern(p, s, r->row, tag, target);
    }
}

// Transfer patterns from the rules to the states.
void fillStates(Rules *rules, States *states) {
    for (int i = 0; i < rules->length; i++) {
        fillState(&rules->a[i], states);
    }
}

// Print out a state in original form, as lots of one-pattern rules.
void printState(State *state, States *states) {
    for (int i = 0; i < state->patterns->length; i++) {
        Pattern *p = &state->patterns->a[i];
        printf("%s ", state->name);
        if (p->look) printf("\\");
        if (p->string[0] == '\\') printf("\\");
        if (p->string[0] == '\n') printf("n");
        else if (p->string[0] == ' ') printf("s");
        else printf("%s", p->string);
        printf(" %s", states->a[p->target].name);
        if (p->tag != NONE) printf(" %s", tagNames[p->tag & 0x1F]);
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
    int n = base->patterns->length;
    base->patterns->a[i] = base->patterns->a[n-1];
    base->patterns->length--;
}

// Add a singleton pattern if not already handled.
void addSingle(State *base, Pattern *range, int ch) {
    bool found = false;
    int n = base->patterns->length;
    for (int i = 0; i < n; i++) {
        char *s = base->patterns->a[i].string;
        if (s[0] == ch && s[1] == '\0') { found = true; break; }
    }
    if (found) return;
    base->patterns->length++;
    base->patterns->a[n] = *range;
    base->patterns->a[n].string = singles[ch];
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
    for (int i = 0; i < base->patterns->length; i++) {
        char *s = base->patterns->a[i].string;
        if (! isRange(s)) continue;
        if (index >= 0) {
            char *t = base->patterns->a[index].string;
            if (overlap(s,t)) {
                error("ranges %s %s overlap in %s", s, t, base->name);
            }
            if (subRange(s,t)) index = i;
        }
        else index = i;
    }
    bool success = (index >= 0);
    if (success) {
        Pattern *range = &base->patterns->a[index];
        derange(base, range);
        delete(base, index);
    }
    return success;
}

// Expand all ranges in all states.
void derangeAll(States *states) {
    for (int i = 0; i < states->length; i++) {
        bool found = true;
        while (found) found = derangeState(&states->a[i]);
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

void sort(Patterns *list) {
    int n = list->length;
    for (int i = 1; i < n; i++) {
        Pattern p = list->a[i];
        int j = i - 1;
        while (j >= 0 && compare(list->a[j].string, p.string) > 0) {
            list->a[j + 1] = list->a[j];
            j--;
        }
        list->a[j + 1] = p;
    }
}

void sortAll(States *states) {
    for (int i = 0; i < states->length; i++) {
        State *base = &states->a[i];
        sort(base->patterns);
    }
}

// ---------- Checks -----------------------------------------------------------
// Check that a scanner handles every input unambiguously. Check whether states
// occur at the start of tokens, or after the start, or both. Check that if
// after, \s \n patterns have tags. Check that the scanner doesn't get stuck in
// an infinite loop.

// Check that a state has no duplicate patterns, except for stack pops.
void noDuplicates(State *base) {
    Patterns *list = base->patterns;
    for (int i = 0; i < list->length; i++) {
        Pattern *p = &list->a[i];
        char *s = p->string;
        for (int j = i+1; j < list->length; j++) {
            Pattern *q = &list->a[j];
            char *t = q->string;
            if (strcmp(s,t) != 0) continue;
            if (popTag(p->tag) && popTag(q->tag) && p->tag != q->tag) continue;
            printf("%s %s\n", s, t);
            error("state %s has pattern for %s twice", base->name, s);
        }
    }
}

// Check that a state handles every singleton character.
void complete(State *base) {
    char ch = '\n';
    for (int i = 0; i < base->patterns->length; i++) {
        char *s = base->patterns->a[i].string;
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

// Set the start and after flags deduced from a state's patterns. Return true
// if any changes were caused.
bool deduce(State *base, States *states) {
    bool changed = false;
    for (int i = 0; i < base->patterns->length; i++) {
        Pattern *p = &base->patterns->a[i];
        State *target = &states->a[p->target];
        if (p->tag != NONE) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->tag == NONE && ! p->look) {
            if (! target->after) changed = true;
            target->after = true;
        }
        if (p->tag == NONE && p->look && base->start) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->tag == NONE && p->look && base->after) {
            if (! target->after) changed = true;
            target->after = true;
        }
    }
    return changed;
}

// Set the start & after flags for all states. Continue until no changes.
void deduceAll(States *states) {
    states->a[0].start = true;
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < states->length; i++) {
            changed = changed || deduce(&states->a[i], states);
        }
    }
}

// Check, for a state with the after flag, that \s \n patterns have tags.
void separates(State *base) {
    if (! base->after) return;
    for (int i = 0; i < base->patterns->length; i++) {
        Pattern *p = &base->patterns->a[i];
        if (p->string[0] == ' ' || p->string[0] == '\n') {
            if (p->tag == NONE) {
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
void follow(States *states, State *base, char *look) {
    if (base->visited) error("state %s can loop", base->name);
    base->visited = true;
    for (int i = 0; i < base->patterns->length; i++) {
        if (! base->patterns->a[i].look) continue;
        char *s = base->patterns->a[i].string;
        if (s[0] == ' ' || s[0] == '\n') continue;
        if (s[0] > look[0]) break;
        if (s[0] < look[0]) continue;
        char *next;
        if (prefix(s, look)) next = look;
        else if (prefix(look, s)) next = s;
        else continue;
        State *target = &states->a[base->patterns->a[i].target];
        follow(states, target, next);
    }
    base->visited = false;
}

// Start a search from a given state, for each possible input character.
void search(States *states, State *base) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if ('\n' < ch && ch < ' ') continue;
        char *look = singles[ch];
        follow(states, base, look);
    }
}

void checkAll(States *states) {
    for (int i = 0; i < states->length; i++) {
        noDuplicates(&states->a[i]);
        complete(&states->a[i]);
    }
    deduceAll(states);
    for (int i = 0; i < states->length; i++) {
         separates(&states->a[i]);
         search(states, &states->a[i]);
    }
}

int main() {
    char *text = readFile("c.txt");
    normalize(text);
    Strings *lines = splitLines(text);
    Rules *rules = getRules(lines);
    States *states = makeStates(rules);
    fillStates(rules, states);
    derangeAll(states);
    sortAll(states);
//    printState(&states->a[1], states);
    checkAll(states);


    for (int i = 0; i < states->length; i++) free(states->a[i].patterns);
    free(states);
    for (int i = 0; i < rules->length; i++) free(rules->a[i].strings);
    free(rules);
    free(lines);
    free(text);
}

// TODO: push and pop now simpler. Still need a flag for 'last'.

/*
//==============================================================================

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

// Flags added to the tag in an action. The LINK flag indicates that the action
// is a link to the overflow area. The LOOK flag indicates a lookahead pattern.
enum { LINK = 0x80, LOOK = 0x40 };

// Fill in an action for a simple pattern: [tag,target] with lookahead flag.
void compileAction(byte *action, Pattern *p) {
    int tag = p->tag;
    int target = p->target;
    bool look = p->look;
    if (look) tag = LOOK | tag;
    action[0] = tag;
    action[1] = target;
}

// When there is more than one pattern for a state starting with a character,
// enter the given offset into the table in bigendian order with LINK flag set.
void compileLink(byte *action, int offset) {
    action[0] = LINK | ((offset >> 8) & 0x7F);
    action[1] = offset & 0xFF;
}

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




// Fill in a pattern at the end of the overflow area, returning the new end. The
// pattern is stored as a byte containing the length, followed by the
// characters of the pattern after the first, followed by the action. For
// example <= with tag OP and target t is stored as 4 bytes [2, '=', OP, t].
int compileExtra(Pattern *p, byte *table, int end) {
    int len = strlen(p->match);
    byte *entry = &table[end];
    end = end + len + 2;
    entry[0] = len;
    strncpy((char *)&entry[1], p->match + 1, len - 1);
    compileAction(&entry[len], p);
    return end;
}

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
//   act = 0x80 ? LINK : tag&0x3F
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

// ---------- Arrays -----------------------------------------------------------
// An array has a preceding length. The length is initially zero. An array must
// be allocated with a capacity large enough to cover the maximum length ever
// needed, to avoid reallocation.

// Pointer aligned array descriptor.
struct prefix { int length; void *align[]; };
typedef struct prefix Prefix;

void *array(int max, int unit) {
    Prefix *p = malloc(sizeof(Prefix) + max * unit);
    p->length = 0;
    return p + 1;
}

int length(void *a) {
    Prefix *p = a;
    p = p - 1;
    return p->length;
}

// Increase or decrease length. Return new length.
int adjust(void *a, int delta) {
    Prefix *p = a;
    p = p - 1;
    p->length += delta;
    return p->length;
}

void release(void *a) {
    Prefix *p = a;
    p = p - 1;
    free(p);
}

// Release an array of pointers, and the structures they point to.
void releasep(void *a) {
    void **p = a;
    for (int i = 0; i < length(p); i++) free(p[i]);
    release(a);
}



*/
