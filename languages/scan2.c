// Free and open source, see licence.txt.
// Provide a standalone scanner to test language definitions.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// ---------- Tags -------------------------------------------------------------
// This enumeration of tags and their names must be kept the same as in other
// Snipe components, and kept to a maximum of 64 entries. The default if there
// is no tag is ADD meaning continue the token.

enum tag {
    ADD, ARTICLE, BEGIN0, BEGIN1, BEGIN2, BEGIN3, COMMENT, END0, END1, END2,
    END3, FUNCTION, GAP, ID, JOIN, KEY, NEWLINE, OP0, OP1, OP2, OP3, NOTE,
    PROPERTY, QUOTE, RESERVED, SIGN0, SIGN1, SIGN2, SIGN3, TYPE, UNKNOWN,
    VALUE
};

char *tagNames[64] = {
    [ADD]="ADD",[ARTICLE]="ARTICLE", [BEGIN0]="BEGIN0", [BEGIN1]="BEGIN1",
    [BEGIN2]="BEGIN2", [BEGIN3]="BEGIN3", [COMMENT]="COMMENT", [END0]="END0",
    [END1]="END1", [END2]="END2", [END3]="END3", [FUNCTION]="FUNCTION",
    [GAP]="GAP", [ID]="ID",[JOIN]="JOIN", [KEY]="KEY", [NEWLINE]="NEWLINE",
    [OP0]="OP0",[OP1]="OP1",[OP2]="OP2", [OP3]="OP3",[NOTE]="NOTE",
    [PROPERTY]="PROPERTY", [QUOTE]="QUOTE",[RESERVED]="RESERVED",
    [SIGN0]="SIGN0",[SIGN1]="SIGN1",[SIGN2]="SIGN2", [SIGN3]="SIGN3",
    [TYPE]="TYPE",[UNKNOWN]="UNKNOWN",[VALUE]="VALUE"
};

int findTag(char *name) {
    for (int i = 0; i < 64; i++) {
        if (tagNames[i] == NULL) continue;
        if (strcmp(tagNames[i], name) == 0) return i;
    }
    return -1;
}

// ---------- Arrays -----------------------------------------------------------
// Arrays are allocated with a preceding size (aligned, initially 0). A maximum
// capacity is given for a variable-size array, to avoid reallocation.

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

void resizeBy(void *a, int d) {
    resize(a, size(a) + d);
}

void *freeze(void *a, int n) {
    void *p = (char *) a - ALIGN;
    p = realloc(p, ALIGN + n);
    return (char *)p + ALIGN;
}

void release(void *a) {
    char *p = (char *)a - ALIGN;
    free(p);
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

// ---------- Lines ------------------------------------------------------------
// Read in a language description as a character array, normalize, and split the
// text into lines, in place.

// Read file as string, ignore I/O errors, add final newline if necessary.
char *readFile(char const *path) {
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
// Extract the rules from the text, each rule being an array of tokens.

// A rule is a line number and an array of tokens.
struct rule { int row; char **tokens; };
typedef struct rule rule;

// Split a line into tokens, adding ADD for a rule with no tag.
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
    bool hasTag = isupper(tokens[n-1][0]);
    if (! hasTag) tokens[n++] = "ADD";
    resize(tokens, n);
    return tokens;
}

// Extract the rules from the lines as arrays of tokens.
rule *getRules(char **lines) {
    rule *rules = allocate(size(lines) * sizeof(rule));
    int n = 0;
    for (int i = 0; i < size(lines); i++) {
        bool isRule = islower(lines[i][0]);
        if (! isRule) continue;
        rules[n++] = (rule) { .row = i+1, .tokens = splitTokens(lines[i]) };
    }
    resize(rules, n);
    return rules;
}

// Count up the patterns belonging to a given state. Add 96 for possible
// additional one-character patterns when ranges are expanded.
int countPatterns(rule *rules, char *name) {
    int n = 96;
    for (int i = 0; i < size(rules); i++) {
        char **tokens = rules[i].tokens;
        if (strcmp(tokens[0], name) != 0) continue;
        n = n + size(tokens) - 3;
    }
    return n;
}

// ---------- States and patterns ----------------------------------------------
// Convert the rules into an array of states with patterns.

// A pattern is a string to be matched, and the action to take on matching it
// (i.e. add to the token, give it a type, jump to the target state).
struct pattern { char *match; bool lookahead; int tag; int target; };
typedef struct pattern pattern;

// A state has a name, an array of patterns, and flags to say whether the state
// can occur at the start of a token, or can occur after the start. A visited
// flag helps check for cycles of states.
struct state { char *name; pattern *patterns; bool starts, resumes, visited; };
typedef struct state state;

// Find an existing state by name, returning its index or -1.
int findState(state *states, char *name) {
    int n = size(states);
    for (int i = 0; i < n; i++) {
        if (strcmp(states[i].name, name) == 0) return i;
    }
    return -1;
}

// Add a new blank state with the given name.
void addState(state *states, char *name, int maxPatterns) {
    int n = size(states);
    pattern *patterns = allocate(maxPatterns * sizeof(pattern));
    resizeBy(states, 1);
    states[n] = (state) {
        .name=name, .patterns=patterns,
        .starts=false, .resumes=false, .visited=false
    };
}

// Convert a string, target and tag to a pattern. Take off a backslash
// indicating a lookahead, and convert a double backslash into a single.
void convert(char *s, int target, int tag, pattern *p) {
    p->target = target;
    p->tag = tag;
    p->lookahead = false;
    if (s[0] == '\\' && (s[1] != '\\' || s[2] == '\\')) {
        p->lookahead = true;
        s = &s[1];
        if (strcmp(s, "s") == 0) s[0] = ' ';
        if (strcmp(s, "n") == 0) s[0] = '\n';
    }
    else if (s[0] == '\\' && s[1] == '\\') s = &s[1];
    p->match = s;
}

// Create empty base states from the rules.
state *makeStates(rule *rules) {
    state *states = allocate(size(rules) * sizeof(state));
    for (int i = 0; i < size(rules); i++) {
        rule *r = &rules[i];
        char **tokens = r->tokens;
        int n = size(tokens);
        char *name = tokens[0];
        char *targetName = tokens[n-2];
        if (n < 4) error("incomplete rule on line %d", r->row);
        if (! islower(targetName[0])) {
            error("expecting target state on line %d", r->row);
        }
        if (findState(states, name) < 0) {
            addState(states, name, countPatterns(rules, name));
        }
    }
    return states;
}

// Transfer patterns from a rule into its base state.
void fillState(rule *r, state *states) {
    char **tokens = r->tokens;
    int n = size(tokens);
    int index = findState(states, tokens[0]);
    state *st = &states[index];
    char *targetName = tokens[n-2];
    char *tagName = tokens[n-1];
    int tag = findTag(tagName);
    if (tag < 0) error("unknown tag %s on line %d", tagName, r->row);
    int target = findState(states, targetName);
    if (target < 0) {
        error("unknown target state %s on line %d", targetName, r->row);
    }
    int count = size(st->patterns);
    for (int i = 1; i < n-2; i++) {
        char *t = tokens[i];
        if (strcmp(t, "\\") == 0) error("empty lookahead on line %d", r->row);
        if (t[0] == '\\' && 'a' <= t[1] && t[1] <= 'z' && t[2] == '\0') {
            if (t[1] != 's' && t[1] != 'n') {
                error("bad lookahead on line %d", r->row);
            }
        }
        convert(t, target, tag, &st->patterns[count++]);
    }
    resize(st->patterns, count);
}

// Transfer patterns from the rules to the states.
void fillStates(rule *rules, state *states) {
    for (int i = 0; i < size(rules); i++) fillState(&rules[i], states);
}

// ---------- Ranges -----------------------------------------------------------
// Expand ranges such as 0..9 to several one-character patterns, with more
// specific patterns (subranges and individual characters) taking precedence.

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

// Remove the i'th pattern from a state's patterns. The patterns are not yet
// sorted, so replace it with the last pattern.
pattern extract(state *base, int i) {
    int n = size(base->patterns);
    pattern p = base->patterns[i];
    base->patterns[i] = base->patterns[n-1];
    resize(base->patterns, n-1);
    return p;
}

// Expand a state's range into singles, and add them if not already handled.
void derange(state *base, pattern *range) {
    char *s = range->match;
    int n = size(base->patterns);
    for (int ch = s[0]; ch <= s[3]; ch++) {
        bool found = false;
        for (int i = 0; i < size(base->patterns); i++) {
            char *s = base->patterns[i].match;
            if (s[0] == ch && s[1] == '\0') { found = true; break; }
        }
        if (found) continue;
        base->patterns[n] = *range;
        base->patterns[n].match = singles[ch];
        n++;
    }
    resize(base->patterns, n);
}

// For a given state, find a most specific range, expand it, return success.
bool derangeState(state *base) {
    int index = -1;
    for (int i = 0; i < size(base->patterns); i++) {
        char *s = base->patterns[i].match;
        if (! isRange(s)) continue;
        if (index >= 0) {
            char *t = base->patterns[index].match;
            if (overlap(s,t)) {
                error("ranges %s %s overlap in %s", s, t, base->name);
            }
            if (subRange(t,s)) index = i;
        }
        else index = i;
    }
    if (index < 0) return false;
    pattern range = extract(base, index);
    derange(base, &range);
    return true;
}

// Expand all ranges in all states.
void derangeAll(state *states) {
    for (int i = 0; i < size(states); i++) {
        bool found = true;
        while (found) found = derangeState(&states[i]);
    }
}

// ---------- Sorting ----------------------------------------------------------
// Sort the patterns for each state into lexicographic order, except that if s
// is a prefix of t, t comes before s.

bool prefix(char *s, char *t) {
    if (strlen(s) >= strlen(t)) return false;
    if (strncmp(s, t, strlen(s)) == 0) return true;
    return false;
}

int compare(char *s, char *t) {
    if (prefix(s,t)) return 1;
    if (prefix(t,s)) return -1;
    return strcmp(s,t);
}

void sort(pattern *list) {
    int n = size(list);
    for (int i = 1; i < n; i++) {
        pattern p = list[i];
        int j = i - 1;
        while (j >= 0 && compare(list[j].match, p.match) > 0) {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = p;
    }
}

void sortAll(state *states) {
    for (int i = 0; i < size(states); i++) {
        state *base = &states[i];
        sort(base->patterns);
    }
}

// ---------- Checks -----------------------------------------------------------
// Check that a scanner handles every input unambiguously. Also check that it
// doesn't get stuck in an infinite loop or produce empty tokens, by following
// jumps, i.e. lookahead patterns which transfer control to a target state
// without consuming input. 

// Scan the states to set their flags. Set the starts flag for a state which
// can occur at the start of a token, and the resumes flag for a state which can
// occur after the start.
void scanPatterns(state *states) {
    states[0].starts = true;
    for (int s = 0; s < size(states); s++) {
        state *base = &states[s];
        pattern *list = base->patterns;
        for (int j = 0; j < size(list); j++) {
            pattern *p = &list[j];
            state *target = &states[p->target];
            if (p->tag != ADD) target->starts = true;
            else if (! p->lookahead) target->resumes = true;
        }
    }
}

// Check that a state has no duplicate patterns.
void noDuplicates(state *base) {
    pattern *list = base->patterns;
    for (int i = 0; i < size(list); i++) {
        char *s = list[i].match;
        for (int j = i+1; j < size(list); j++) {
            char *t = list[j].match;
            if (strcmp(s,t) != 0) continue;
            error("state %s has pattern %s twice", base->name, s);
        }
    }
}

// Check that a state handles every singleton character. If a state can only
// occur at the start of a token, it need not handle \s or \n. 
void complete(state *base) {
    noDuplicates(base);
    char ch = base->resumes ? '\n' : '!';
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

void allComplete(state *states) {
    for (int i = 0; i < size(states); i++) complete(&states[i]);
}

// Search for a chain of lookaheads from a given state which can cause an
// infinite loop or create an empty token. The look argument is the longest
// lookahead in the chain so far, to ensure the lookaheads in the chain are all
// compatible. The atStart argument indicates whether the search started from a
// possible start of token.
void follow(state *states, state *base, char *look, bool atStart) {
    if (base->visited) error("state %s can loop", base->name);
    base->visited = true;
    for (int i = 0; i < size(base->patterns); i++) {
        if (! base->patterns[i].lookahead) continue;
        char *s = base->patterns[i].match;
        if (s[0] > look[0]) break;
        if (s[0] < look[0]) continue;
        char *next;
        if (prefix(s, look)) next = look;
        else if (prefix(look, s)) next = s;
        else continue;
        if (atStart && base->patterns[i].tag != ADD) {
            error("state %s can create an empty token", base->name);
        }
        state *target = &states[base->patterns[i].target];
        follow(states, target, next, atStart);
    }
    base->visited = false;
}

// Start a search from a given state, for each possible input character. If the
// state can only occur at the start of a token, exclude \n and \s.
void search(state *states, state *base) {
    char ch0 = base->resumes ? '\n' : '!';
    for (int ch = ch0; ch <= '~'; ch++) {
        char *look = singles[ch];
        follow(states, base, look, base->starts);
    }
}

void searchAll(state *states) {
    for (int i = 0; i < size(states); i++) search(states, &states[i]);
}

// ---------- Compiling --------------------------------------------------------
// Compile the states into a compact transition table. The table has a row for
// each state, followed by an overflow area for patterns which are more than
// one character long. Each row consists of 96 entries of two bytes each, one
// for each character \s !..~ \n. The scanner uses the current state and the
// next character in the source text to look up an entry. The entry may be an
// action for that single character, or an offset relative to the start of the
// table to a list of patterns starting with that character, with their
// actions.

typedef unsigned char byte;

// Fill in an action for a given pattern, as two bytes, one for the tag and one
// for the target state. The tag has a bit 0x40 added to indicate a lookahead
// action. The top bit 0x80 is zero.
void compileAction(byte *action, pattern *p) {
    int code = p->tag;
    if (p->lookahead) code += 0x40;
    action[0] = code;
    action[1] = p->target; 
}

// When there is more than one pattern for a state starting with a character,
// enter the given offset into the table in bigendian order with 0x80 set.
void compileLink(byte *action, int offset) {
    action[0] = ((offset >> 8) & 0xFF) + 0x80;
    action[1] = offset & 0xFF;
}

// Fill in a pattern after the end of the table. It is stored as an action,
// followed by a byte containing the number of characters in the pattern after
// the first, followed by those characters. (A group of patterns after the
// table needs no termination because the last entry is always a singleton
// which matches.)
void compileExtra(pattern *p, byte *table) {
    int len = strlen(p->match);
    byte *entry = &table[size(table)];
    resizeBy(table, 2 + 1 + len - 1);
    compileAction(entry, p);
    entry[2] = len - 1;
    strncpy((char *)&entry[4], p->match + 1, len - 1);
}

// Fill the patterns from a state into a row of the table. 
void compileState(state *base, byte *table, int row) {
    pattern *patterns = base->patterns;
    int n = size(patterns), i = 0;
    while (i < n) {
        pattern *p = &patterns[i];
        char *s = p->match;
        int col = s[0] == '\n' ? 95 : s[0] - ' ';
        int entry = 2 * (96 * row + col);
        if (strlen(s) == 1) {
            compileAction(&table[entry], p);
            i++;
        }
        else {
            char ch = s[0];
            compileLink(&table[entry], size(table));
            while (i < n && patterns[i].match[0] == ch) {
                compileExtra(&patterns[i], table);
                i++;
            }
        }
    }
}

void compile(state *states, byte *table) {
    for (int i = 0; i < size(states); i++) compileState(&states[i], table, i);
//    freeze(table, size(table));
}

int main() {
    clock_t t;
    t = clock();
    char *text = readFile("c.txt");
    normalize(text);
    char **lines = splitLines(text);
    rule *rules = getRules(lines);
    printf("#rules: %d\n", size(rules));
    state *states = makeStates(rules);
    printf("#states: %d\n", size(states));
    fillStates(rules, states);
    derangeAll(states);
    sortAll(states);
    scanPatterns(states);
    allComplete(states);
    searchAll(states);
    byte *table = allocate(96*size(states) + 10000);
    compile(states, table);
//    for (int i = 1; i < 2; i++) {
//        printf("State %s: %d patterns\n", states[i].name, size(states[i].patterns));
//        for (int j = 0; j < size(states[i].patterns); j++) {
//            printf("%s\n", states[i].patterns[j].match);
//        }
//    }

    release(table);
    for (int i = 0; i < size(states); i++) release(states[i].patterns);
    release(states);
    for (int i = 0; i < size(rules); i++) release(rules[i].tokens);
    release(rules);
    release(lines);
    release(text);
    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("The program took %f seconds to execute\n", time_taken);
}
