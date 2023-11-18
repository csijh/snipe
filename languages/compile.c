// Free and open source, see licence.txt.

// Compile a language definition. Read in a file such as c.txt, check the rules
// for consistency, run the tests and, if everything succeeds, write out a
// compact state table in binary file c.bin.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// TODO: get rid of WRONG. Consider bracket matching for unclosed quotes.
// TODO: ADD for added semicolon.

// ---------- Tags -------------------------------------------------------------
// These tags and their names must be kept the same as in other Snipe modules. A
// tag represents an action to take in the state machine. The first 26 are the
// ones which can be used in language definitions (with unused gaps). They
// represent both the action of terminating a token, and the type to mark it
// with. GAP and NEWLINE are used in a similar way to mark spaces and newlines
// as separators, and NONE represents the action of continuing a token, and is
// used to mark token characters after the first. PUSH, POP and MISS are
// actions on the stack of unmatched open brackets. LINK is used to move to the
// state machine's overflow area.

enum tag {
    A, BEGIN, COMMENT, DOCUMENT, END, FUNCTION, G, H, IDENTIFIER, JOIN, KEYWORD,
    LEFT, MARK, NOTE, OP, PROPERTY, QUOTE, RIGHT, SIGN, TYPE, UNARY, VALUE,
    WRONG, X, Y, Z, GAP, NEWLINE, NONE, PUSH, POP, MISS, LINK
};

// The first character is used in tests.
char *tagNames[64] = {
    "?", "BEGIN", "COMMENT", "DOCUMENT", "END", "FUNCTION", "?", "?",
    "IDENTIFIER", "JOIN", "KEYWORD", "LEFT", "MARK", "NOTE", "OPERATOR",
    "PROPERTY", "QUOTE", "RIGHT", "SIGN", "TYPE", "UNARY", "VALUE", "WRONG",
    "?", "?", "?", " ", ".", "-", "?", "?", "?", "?"
};

bool prefix(char *s, char *t) {
    if (strlen(s) >= strlen(t)) return false;
    if (strncmp(s, t, strlen(s)) == 0) return true;
    return false;
}

// Find a user-visible tag or abbreviation by name.
int findTag(char *name) {
    if (strcmp(name, "TAG") == 0) name = "TYPE";
    for (int i = A; i <= Z; i++) {
        if (tagNames[i][0] == '?') continue;
        if (strcmp(name, tagNames[i]) == 0) return i;
        if (prefix(name, tagNames[i])) return i;
    }
    return -1;
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

// ---------- Arrays -----------------------------------------------------------
// Arrays are allocated with a preceding size (aligned, initially 0). An array
// is assumed to be allocated with a capacity large enough to cover the maximum
// size ever needed, to avoid reallocation.

enum { ALIGN = alignof(max_align_t) };

void *allocate(int max) {
    void *p = malloc(ALIGN + max);
    ((int *) p)[0] = 0;
    return (char *)p + ALIGN;
}

int size(void *a) {
    void *p = (char *) a - ALIGN;
    return ((int *) p)[0];
}

void resize(void *a, int n) {
    void *p = (char *) a - ALIGN;
    ((int *) p)[0] = n;
}

void release(void *a) {
    char *p = (char *)a - ALIGN;
    free(p);
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
    char *text =  allocate(size + 2);
    fread(text, 1, size, file);
    if (text[size-1] != '\n') text[size++] = '\n';
    text[size] = '\0';
    resize(text, size);
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
    char **lines = allocate(n * sizeof(char *));
    resize(lines, n);
    n = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i] = '\0';
        char *line = trim(&text[start]);
        lines[n++] = line;
        start = i + 1;
    }
    return lines;
}

// ---------- Rules ------------------------------------------------------------
// Extract the rules from the text, as tokens.

// A rule is a line number and a collection of tokens.
struct rule { int row; char *base, *target, *tag, *op; char **patterns; };
typedef struct rule Rule;

// Split a line into tokens in place.
char **splitTokens(char *line) {
    int len = strlen(line), n = 0, start = 0;
    char **tokens = allocate(len * sizeof(char *));
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] != ' ') continue;
        line[i] = '\0';
        if (i > 0 && line[i-1] != '\0') tokens[n++] = &line[start];
        start = i + 1;
    }
    tokens[n++] = &line[start];
    resize(tokens, n);
    return tokens;
}

// Make a rule from an array of tokens.
void makeRule(Rule *rule, int row, char **tokens) {
    rule->row = row;
    int n = size(tokens);
    if (n < 3) error("incomplete rule on line %d", row);
    rule->base = tokens[0];
    for (int i = 1; i < n; i++) tokens[i-1] = tokens[i];
    resize(tokens, --n);
    char *t = tokens[n-1], *t2 = tokens[n-2];
    if (t[0] == '+' || t[0] == '-') {
        if (! isupper(t2[0])) {
            error("push or pop should follow tag on line %d", row);
        }
        rule->op = t;
        resize(tokens, --n);
    }
    else rule->op = NULL;
    t = tokens[n-1];
    if (isupper(t[0])) {
        int tag = findTag(t);
        if (tag < 0) error("unknown tag %s on line %d", t, row);
        rule->tag = t;
        resize(tokens, --n);
    }
    else rule->tag = NULL;
    if (n < 2) error("incomplete rule on line %d", row);
    t = tokens[n-1];
    if (! islower(t[0])) error("expecting target state on line %d", row);
    rule->target = t;
    resize(tokens, --n);
    rule->patterns = tokens;
}

// Extract the rules from the lines.
Rule *getRules(char **lines) {
    Rule *rules = allocate(size(lines) * sizeof(Rule));
    int n = 0;
    for (int i = 0; i < size(lines); i++) {
        bool isRule = islower(lines[i][0]);
        if (! isRule) continue;
        makeRule(&rules[n++], i+1, splitTokens(lines[i]));
    }
    resize(rules, n);
    return rules;
}

// Count up the patterns belonging to a given state. Add 96 for possible
// additional one-character patterns when ranges are expanded.
int countPatterns(Rule *rules, char *name) {
    int n = 96;
    for (int i = 0; i < size(rules); i++) {
        if (strcmp(name, rules[i].base) != 0) continue;
        n = n + size(rules[i].patterns);
    }
    return n;
}

// ---------- States and patterns ----------------------------------------------
// Convert the rules into an array of states with patterns.

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

// A pattern is a string to be matched, and the action to take (i.e. maybe add
// to the token, maybe terminate it, jump to the target state).
struct pattern { char *original, *match; bool lookahead; int tag; int target; };
typedef struct pattern Pattern;

// A state has a name, and an array of patterns. It has flags to say whether it
// occurs at or after the start of tokens. It has a visited flag used when
// checking for infinite loops.
struct state { char *name; Pattern *patterns; bool start, after, visited; };
typedef struct state State;

// Find an existing state by name, returning its index or -1.
int findState(State *states, char *name) {
    int n = size(states);
    for (int i = 0; i < n; i++) {
        if (strcmp(states[i].name, name) == 0) return i;
    }
    return -1;
}

// Add a new blank state with the given name.
void addState(State *states, char *name, int maxPatterns) {
    int n = size(states);
    Pattern *patterns = allocate(maxPatterns * sizeof(Pattern));
    resize(states, n+1);
    states[n] = (State) {
        .name=name, .patterns=patterns,
        .start=false, .after=false, .visited=false
    };
}

//==============================================================================
// TODO: add (s SP s GAP) before (s \s t ...) and similarly for \n.
// These only match at start of token.
// TODO: convert +t -t to PUSH, POP and add a MISS pattern.

// Convert a string, target and tag to a pattern. Take off a backslash
// indicating a lookahead, and convert a double backslash into a single.
void convert(char *s, int target, int tag, Pattern *p) {
    p->original = s;
    p->target = target;
    p->tag = tag;
    p->lookahead = false;
    if (s[0] == '\\' && (s[1] != '\\' || s[2] == '\\')) {
        p->lookahead = true;
        s = &s[1];
        if (strcmp(s, "s") == 0) s = singles[' '];
        if (strcmp(s, "n") == 0) s = singles['\n'];
    }
    else if (s[0] == '\\' && s[1] == '\\') s = &s[1];
    p->match = s;
}

// Create empty base states from the rules.
State *makeStates(Rule *rules) {
    State *states = allocate(size(rules) * sizeof(State));
    for (int i = 0; i < size(rules); i++) {
        Rule *r = &rules[i];
        if (findState(states, r->base) < 0) {
            addState(states, r->base, countPatterns(rules, r->base));
        }
    }
    return states;
}

// Transfer patterns from a rule into its base state. Expand \ as \NL..~
void fillState(Rule *r, State *states) {
    int index = findState(states, r->base);
    State *base = &states[index];
    int target = findState(states, r->target);
    int tag = (r->tag == NULL) ? NONE : findTag(r->tag);
    if (target < 0) {
        error("unknown target state %s on line %d", r->target, r->row);
    }
    char **patterns = r->patterns;
    int n = size(patterns);
    int count = size(base->patterns);
    for (int i = 0; i < n; i++) {
        char *t = patterns[i];
        if (strcmp(t, "\\") == 0) {
            convert("\\\n..~", target, tag, &base->patterns[count++]);
            continue;
        }
        if (t[0] == '\\' && 'a' <= t[1] && t[1] <= 'z' && t[2] == '\0') {
            if (t[1] != 's' && t[1] != 'n') {
                error("bad lookahead on line %d", r->row);
            }
        }
        convert(t, target, tag, &base->patterns[count++]);
    }
    resize(base->patterns, count);
}

// Transfer patterns from the rules to the states.
void fillStates(Rule *rules, State *states) {
    for (int i = 0; i < size(rules); i++) fillState(&rules[i], states);
}

// ---------- Ranges -----------------------------------------------------------
// Expand ranges such as 0..9 to several one-character patterns, with more
// specific patterns (subranges and individual characters) taking precedence.
// A range may be x..~ where x is '\n' to represent the \ abbreviation.

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
Pattern extract(State *base, int i) {
    int n = size(base->patterns);
    Pattern p = base->patterns[i];
    base->patterns[i] = base->patterns[n-1];
    resize(base->patterns, n-1);
    return p;
}

// Add a singleton pattern if not already handled.
void addSingle(State *base, Pattern *range, int ch) {
    bool found = false;
    for (int i = 0; i < size(base->patterns); i++) {
        char *s = base->patterns[i].match;
        if (s[0] == ch && s[1] == '\0') { found = true; break; }
    }
    if (found) return;
    int n = size(base->patterns);
    resize(base->patterns, n + 1);
    base->patterns[n] = *range;
    base->patterns[n].match = singles[ch];
}

// Expand a state's range into singles, and add them if not already handled.
void derange(State *base, Pattern *range) {
    char *s = range->match;
    for (int ch = s[0]; ch <= s[3]; ch++) {
        if (ch == '\n' || ch >= ' ') addSingle(base, range, ch);
    }
}

// For a given state, find a most specific range, expand it, return success.
bool derangeState(State *base) {
    int index = -1;
    for (int i = 0; i < size(base->patterns); i++) {
        char *s = base->patterns[i].match;
        if (! isRange(s)) continue;
        if (index >= 0) {
            char *t = base->patterns[index].match;
            if (overlap(s,t)) {
                error("ranges %s %s overlap in %s", s, t, base->name);
            }
            if (subRange(s,t)) index = i;
        }
        else index = i;
    }
    if (index < 0) return false;
    Pattern range = extract(base, index);
    derange(base, &range);
    return true;
}

// Expand all ranges in all states.
void derangeAll(State *states) {
    for (int i = 0; i < size(states); i++) {
        bool found = true;
        while (found) found = derangeState(&states[i]);
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

void sort(Pattern *list) {
    int n = size(list);
    for (int i = 1; i < n; i++) {
        Pattern p = list[i];
        int j = i - 1;
        while (j >= 0 && compare(list[j].match, p.match) > 0) {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = p;
    }
}

void sortAll(State *states) {
    for (int i = 0; i < size(states); i++) {
        State *base = &states[i];
        sort(base->patterns);
    }
}

// ---------- Checks -----------------------------------------------------------
// Check that a scanner handles every input unambiguously. Check whether states
// occur at the start of tokens, or after the start, or both. Check that if
// after, \s \n patterns have tags. Check that the scanner doesn't get stuck in
// an infinite loop.

// Check that a state has no duplicate patterns.
// TODO except pops.
void noDuplicates(State *base) {
    Pattern *list = base->patterns;
    for (int i = 0; i < size(list); i++) {
        Pattern *p = &list[i];
        char *s = p->match;
        for (int j = i+1; j < size(list); j++) {
            char *t = list[j].match;
            if (strcmp(s,t) != 0) continue;
            error("state %s has pattern for %s twice", base->name, p->match);
        }
    }
}

// Check that a state handles every singleton character.
void complete(State *base) {
    char ch = '\n';
    for (int i = 0; i < size(base->patterns); i++) {
        char *s = base->patterns[i].match;
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
bool deduce(State *base, State *states) {
    bool changed = false;
    for (int i = 0; i < size(base->patterns); i++) {
        Pattern *p = &base->patterns[i];
        State *target = &states[p->target];
        if (p->tag != NONE) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->tag == NONE && ! p->lookahead) {
            if (! target->after) changed = true;
            target->after = true;            
        }
        if (p->tag == NONE && p->lookahead && base->start) {
            if (! target->start) changed = true;
            target->start = true;
        }
        if (p->tag == NONE && p->lookahead && base->after) {
            if (! target->after) changed = true;
            target->after = true;
        }
    }
    return changed;
}

// Set the start & after flags for all states. Continue until no changes.
void deduceAll(State *states) {
    states[0].start = true;
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < size(states); i++) {
            changed = changed || deduce(&states[i], states); 
        }
    }
}

// Check, for a state with the after flag, that \s \n patterns have tags.
void separates(State *base) {
    if (! base->after) return;
    for (int i = 0; i < size(base->patterns); i++) {
        Pattern *p = &base->patterns[i];
        if (p->match[0] == ' ' || p->match[0] == '\n') {
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
void follow(State *states, State *base, char *look) {
    if (base->visited) error("state %s can loop", base->name);
    base->visited = true;
    for (int i = 0; i < size(base->patterns); i++) {
        if (! base->patterns[i].lookahead) continue;
        char *s = base->patterns[i].match;
        if (s[0] == ' ' || s[0] == '\n') continue;
        if (s[0] > look[0]) break;
        if (s[0] < look[0]) continue;
        char *next;
        if (prefix(s, look)) next = look;
        else if (prefix(look, s)) next = s;
        else if (strcmp(s, look) == 0) next = s;
        else continue;
        State *target = &states[base->patterns[i].target];
        follow(states, target, next);
    }
    base->visited = false;
}

// Start a search from a given state, for each possible input character.
void search(State *states, State *base) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if ('\n' < ch && ch < ' ') continue;
        char *look = singles[ch];
        follow(states, base, look);
    }
}

void checkAll(State *states) {
    for (int i = 0; i < size(states); i++) {
        noDuplicates(&states[i]);
        complete(&states[i]);
    }
    deduceAll(states);
    for (int i = 0; i < size(states); i++) {
        separates(&states[i]);
        search(states, &states[i]);
    }
}

//==============================================================================

// ---------- Compiling --------------------------------------------------------
// Compile the states into a compact transition table. The table has a row for
// each state, followed by an overflow area used when there is more than one
// pattern for a particular character. Each row consists of 96 entries of two
// bytes each, one for each character \s, !, ..., ~, 0x7F where 0x7F is used to
// represent \n. Spaces and newlines are handled by changing \s and \n patterns
// with tag NONE to non-lookahead patterns with tag GAP or NEWLINE. The scanner
// uses the current state and the next character in the source text to look up
// an entry. The entry may be an action for that single character, or an offset
// relative to the start of the table to a list of patterns starting with that
// character, with their actions.

typedef unsigned char byte;

// Flags added to the tag in an action. The LINKX flag indicates that the action
// is a link to the overflow area. The LOOK flag indicates a lookahead pattern.
enum { LINKX = 0x80, LOOK = 0x40 };

// Fill in an action for a given pattern, as two bytes, one for the tag and one
// for the target state.
void compileAction(byte *action, Pattern *p) {
    int tag = p->tag;
    int target = p->target;
    bool look = p->lookahead;
//    if (tag == NONE && p->match[0] == ' ') { tag = GAP; look = false; }
//    if (tag == NONE && p->match[0] == '\n') { tag = NEWLINE; look = false; }
    if (look) tag = LOOK | tag;
    action[0] = tag;
    action[1] = target; 
}

// When there is more than one pattern for a state starting with a character,
// enter the given offset into the table in bigendian order with LINKX flag set.
void compileLink(byte *action, int offset) {
    action[0] = LINKX | ((offset >> 8) & 0x7F);
    action[1] = offset & 0xFF;
}

// Fill in a pattern at the end of the overflow area. It is stored as a byte
// containing the length, followed by the characters of the pattern after the
// first, followed by the action. For example <= with tag OP and target t is
// stored as 4 bytes [2, '=', OP, t].
void compileExtra(Pattern *p, byte *table) {
    int len = strlen(p->match);
    int n = size(table);
    byte *entry = &table[n];
    resize(table, n + len + 2);
    entry[0] = len;
    strncpy((char *)&entry[1], p->match + 1, len - 1);
    compileAction(&entry[len], p);
}

// Fill in the patterns from the position in the given array which start with
// the same character. If there is one pattern (necessarily a singleton
// character), put an action in the table. Otherwise, put a link in the table
// and put the patterns and actions in the overflow area. The group of patterns
// doesn't need to be terminated because the last pattern is always a singleton
// character which matches. Return the new index in the array of patterns.
int compileGroup(Pattern *patterns, int n, byte *table, int row) {
    bool immediate = strlen(patterns[n].match) == 1;
    char ch = patterns[n].match[0];
    int col = (ch == '\n') ? 95 : ch - ' ';
    byte *entry = &table[2 * (96 * row + col)]; 
    if (immediate) {
        compileAction(entry, &patterns[n]);
        return n+1;
    }
    compileLink(entry, size(table));
    while (n < size(patterns) && patterns[n].match[0] == ch) {
        compileExtra(&patterns[n], table);
        n++;
    }
    return n;
}

// Fill all the patterns from all states into the table. 
void compile(State *states, byte *table) {
    for (int i = 0; i < size(states); i++) {
        State *base = &states[i];
        Pattern *patterns = base->patterns;
        int n = 0;
        while (n < size(patterns)) {
            n = compileGroup(patterns, n, table, i);
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
int scan(byte *table, int st, char *in, byte *out, State *states, bool trace) {
    int n = strlen(in);
    for (int i = 0; i<=n; i++) out[i] = NONE;
    int at = 0, start = 0;
    while (at <= n) {
        char ch = in[at];
        if (trace) printf("%s ", states[st].name);
        int col = ch - ' ';
        if (ch == '\0') col = 95;
        byte *action = &table[2 * (96 * st + col)];
        int len = 1;
        if ((action[0] & LINKX) != 0) {
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
    byte *table, int st, char *in, char *expected, State *states, bool trace
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

void runTests(byte *table, char **lines, State *states, bool trace) {
    int st = 0;
    int count = 0;
    for (int i = 0; i < size(lines); i++) {
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
    Rule *rules = getRules(lines);
    State *states = makeStates(rules);
    fillStates(rules, states);
    derangeAll(states);
    sortAll(states);
    checkAll(states);

/*int count = 0;
for (int i = 0; i < size(states); i++) {
    printf("%s: %d %d\n", states[i].name, states[i].start, states[i].after);
    if (states[i].start) count++;
}
printf("#states %d\n", size(states));
printf("%d more needed for starters\n", count);
*/
    byte *table = allocate(2*96*size(states) + 10000);
    resize(table, 2*96*size(states));
    compile(states, table);
    runTests(table, lines, states, trace);

    char outfile[100];
    strcpy(outfile, file);
    strcpy(outfile + strlen(outfile) - 4, ".bin");
    write(outfile, table);

    release(table);
    for (int i = 0; i < size(states); i++) release(states[i].patterns);
    release(states);
    for (int i = 0; i < size(rules); i++) release(rules[i].patterns);
    release(rules);
    release(lines);
    release(text);
}
