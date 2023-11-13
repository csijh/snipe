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

// ---------- Tags -------------------------------------------------------------
// These tags and their names must be kept the same as in other Snipe modules,
// and kept to a maximum of 32 entries. For the first 26, the tag value is the
// same as (ch-'A') where ch is the first letter. There are gaps for unused
// letters. The last 3 tags can't be mentioned in language definitions. The
// default if there is no tag is NONE meaning continue the token.

enum tag {
    A, BEGIN, COMMENT, DOCUMENT, END, FUNCTION, G, H, IDENTIFIER, JOIN, KEYWORD,
    LEFT, MARK, NOTE, OP, PROPERTY, QUOTE, RIGHT, SIGN, TYPE, UNARY, VALUE,
    WRONG, X, Y, Z, GAP, NEWLINE, NONE
};

// The first character is used in tests.
char *tagNames[32] = {
    "?", "BEGIN", "COMMENT", "DOCUMENT", "END", "FUNCTION", "?", "?",
    "IDENTIFIER", "JOIN", "KEYWORD", "LEFT", "MARK", "NOTE", "OP", "PROPERTY",
    "QUOTE", "RIGHT", "SIGN", "TYPE", "UNARY", "VALUE", "WRONG", "?", "?", "?",
    "_", ".", " "
};

void error(char *format, ...) {
    va_list args;
    fprintf(stderr, "Error: ");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, ".\n");
    exit(1);
}

bool prefix(char *s, char *t) {
    if (strlen(s) >= strlen(t)) return false;
    if (strncmp(s, t, strlen(s)) == 0) return true;
    return false;
}

// Find a tag or abbreviation by name, other than NONE, GAP, NEWLINE.
int findTag(char *name) {
    for (int i = 0; i < 64; i++) {
        if (tagNames[i] == NULL) continue;
        if (strcmp(name, tagNames[i]) == 0) return i;
        if (prefix(name, tagNames[i])) return i;
    }
    return -1;
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
// Extract the rules from the text, each rule being an array of tokens.

// A rule is a line number and an array of tokens.
struct rule { int row; char **tokens; };
typedef struct rule rule;

// Split a line into tokens in place, adding NONE for a rule with no tag.
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
    if (! hasTag) tokens[n++] = tagNames[NONE];
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
typedef struct pattern pattern;

// A state has a name, and an array of patterns. It has a flag to say whether it
// occurs at or after the start of tokens. It has a visited flag used when
// checking for infinite loops.
struct state { char *name; pattern *patterns; bool starter, visited; };
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
    resize(states, n+1);
    states[n] = (state) {
        .name=name, .patterns=patterns, .starter=false, .visited=false
    };
}

// Convert a string, target and tag to a pattern. Take off a backslash
// indicating a lookahead, and convert a double backslash into a single.
void convert(char *s, int target, int tag, pattern *p) {
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

// Transfer patterns from a rule into its base state. Expand a \ pattern as
// \ followed by x..~ where x is \n.
void fillState(rule *r, state *states) {
    char **tokens = r->tokens;
    int n = size(tokens);
    int index = findState(states, tokens[0]);
    state *base = &states[index];
    char *targetName = tokens[n-2];
    char *tagName = tokens[n-1];
    int tag = findTag(tagName);
    if (tag < 0) error("unknown tag %s on line %d", tagName, r->row);
    int target = findState(states, targetName);
    if (target < 0) {
        error("unknown target state %s on line %d", targetName, r->row);
    }
    int count = size(base->patterns);
    for (int i = 1; i < n-2; i++) {
        char *t = tokens[i];
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
void fillStates(rule *rules, state *states) {
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
pattern extract(state *base, int i) {
    int n = size(base->patterns);
    pattern p = base->patterns[i];
    base->patterns[i] = base->patterns[n-1];
    resize(base->patterns, n-1);
    return p;
}

// Add a singleton pattern if not already handled.
void addSingle(state *base, pattern *range, int ch) {
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
void derange(state *base, pattern *range) {
    char *s = range->match;
    for (int ch = s[0]; ch <= s[3]; ch++) {
        if (ch == '\n' || ch >= ' ') addSingle(base, range, ch);
    }
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
            if (subRange(s,t)) index = i;
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
// Check that a scanner handles every input unambiguously. Check that states are
// unambiguously starters or continuers. Check that the scanner doesn't get
// stuck in an infinite loop. Check that it doesn't produce empty tokens.

// Check that a state has no duplicate patterns.
void noDuplicates(state *base) {
    pattern *list = base->patterns;
    for (int i = 0; i < size(list); i++) {
        pattern *p = &list[i];
        char *s = p->match;
        for (int j = i+1; j < size(list); j++) {
            char *t = list[j].match;
            if (strcmp(s,t) != 0) continue;
            error("state %s has pattern %s twice", base->name, p->original);
        }
    }
}

// Check that a state handles every singleton character.
void complete(state *base) {
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

// Set the starter flag for a state, if \s has no tag.
void classify(state *base) {
    for (int i = 0; i < size(base->patterns); i++) {
        pattern *p = &base->patterns[i];
        if (p->match[0] != ' ') continue;
        base->starter = (p->tag == NONE);
    }    
}

// Check that the target states reachable from a state have the right starter
// flag. Also check that the state can't create empty tokens.
void look(state *base, state *states) {
    for (int i = 0; i < size(base->patterns); i++) {
        pattern *p = &base->patterns[i];
        state *target = &states[p->target];
        bool ok = true;
        if (p->tag != NONE && ! target->starter) ok = false;
        if (p->tag == NONE) {
            if (! p->lookahead) {
                if (target->starter) ok = false;
            }
            else if (p->match[0] != ' ' && p->match[0] != '\n') {
                if (target->starter != base->starter) ok = false;
            }
        }
        if (! ok) {
            error(
                "according to pattern %s in state %s,\n"
                "state %s can both start and continue tokens",
                p->original, base->name, target->name
            );
        }
        if (base->starter && p->lookahead && p->tag != NONE) {
            error("state %s can create an empty token", base->name);
        }
    }
}

// Search for a chain of lookaheads from a given state which can cause an
// infinite loop. The look argument is the longest lookahead in the chain so
// far, to ensure the lookaheads in the chain are all compatible.
void follow(state *states, state *base, char *look) {
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
        state *target = &states[base->patterns[i].target];
        follow(states, target, next);
    }
    base->visited = false;
}

// Start a search from a given state, for each possible input character.
void search(state *states, state *base) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if ('\n' < ch && ch < ' ') continue;
        char *look = singles[ch];
        follow(states, base, look);
    }
}

void checkAll(state *states) {
    for (int i = 0; i < size(states); i++) {
        noDuplicates(&states[i]);
        complete(&states[i]);
        classify(&states[i]);
    }
    for (int i = 0; i < size(states); i++) {
        look(&states[i], states);
        search(states, &states[i]);
    }
}

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

// Use 0x7F to terminate a line, so that (ch - ' ') can be used as a column
// index in the 96-column table.
enum { EOL = 0x7F };

// Fill in an action for a given pattern, as two bytes, one for the tag and one
// for the target state. The tag has a bit 0x40 added to indicate a lookahead
// action. The top bit 0x80 is zero. NONE becomes GAP or NEWLINE and lookahead
// becomes false for \s or \n.
void compileAction(byte *action, pattern *p) {
    int code = p->tag;
    bool look = p->lookahead;
    if (code == NONE && p->match[0] == ' ') { code = GAP; look = false; }
    if (code == NONE && p->match[0] == '\n') { code = NEWLINE; look = false; }
    if (look) code += 0x40;
    action[0] = code;
    action[1] = p->target; 
}

// When there is more than one pattern for a state starting with a character,
// enter the given offset into the table in bigendian order with 0x80 set.
void compileLink(byte *action, int offset) {
    action[0] = ((offset >> 8) & 0xFF) + 0x80;
    action[1] = offset & 0xFF;
}

// Fill in a pattern at the end of the overflow area. It is stored as a byte
// containing the length, followed by the characters of the pattern after the
// first, followed by the action. For example <= with tag OP and target t is
// stored as 4 bytes [2, '=', OP, t].
void compileExtra(pattern *p, byte *table) {
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
int compileGroup(pattern *patterns, int n, byte *table, int row) {
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
void compile(state *states, byte *table) {
    for (int i = 0; i < size(states); i++) {
        state *base = &states[i];
        pattern *patterns = base->patterns;
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

// Switch on to trace states and pattern matches.
enum { DEBUG = 0 };

// Use the given table and start state to scan the given input line, producing
// the result in the given byte array, and returning the final state. Use
// the states for names in messages.
int scan(byte *table, int st, char *in, byte *out, state *states) {
    int n = strlen(in);
    in[n] = EOL;
    for (int i = 0; i<=n; i++) out[i] = NONE;
    int at = 0, start = 0;
    while (at <= n) {
        char ch = in[at];
        if (DEBUG) printf("%s ", states[st].name);
        int col = ch - ' ';
        byte *action = &table[2 * (96 * st + col)];
        bool immediate = (action[0] & 0x80) == 0;
        int len;
        if (immediate) len = 1;
        else {
            int offset = ((action[0] & 0x7F) << 8) + action[1];
            byte *p = table + offset;
            while (true) {
                len = p[0];
                int i;
                for (i = 1; i < len; i++) {
                    if (in[at + i] != p[i]) break;
                }
                if (i == len) break;
                else p = p + len + 2;
            }
            action = p + len;
        }
        bool lookahead = (action[0] & 0x40) != 0;
        int tag = action[0] & 0x3F;
        if (DEBUG) {
            if (lookahead) printf("\\ ");
            if (in[at] == ' ') printf("SP");
            else if (in[at] == EOL) printf("NL");
            else for (int i = 0; i < len; i++) printf("%c", in[at+i]);
            printf(" %s\n", tagNames[tag]);
        }
        if (! lookahead) at = at + len;
        if (tag != NONE) {
            out[start] = tag;
            start = at;
        }
        st = action[1];
    }
    in[n] = '\0';
    return st;
}

// ---------- Testing ----------------------------------------------------------
// The tests in a language description are intended to check that the rules work
// as expected. They also act as tests for this program. A line starting with >
// is a test and one starting with < is the expected output.

// Carry out a test, given a line of input and an expected line of output.
int runTest(byte *table, int st, char *in, char *expected, state *states) {
    int n = strlen(in) + 1;
    byte bytes[n];
    st = scan(table, st, in+2, bytes+2, states);
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

void runTests(byte *table, char **lines, state *states) {
    int st = 0;
    int count = 0;
    for (int i = 0; i < size(lines); i++) {
        if (lines[i][0] != '>') continue;
        char *in = lines[i];
        char *expected = lines[i+1];
        st = runTest(table, st, in, expected, states);
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
    if (n != 2 || strcmp(args[1] + strlen(args[1]) - 4, ".txt") != 0) {
        printf("Usage: compile c.txt\n");
        exit(0);
    }
    char *text = readFile(args[1]);
    normalize(text);
    char **lines = splitLines(text);
    rule *rules = getRules(lines);
    state *states = makeStates(rules);
    fillStates(rules, states);
    derangeAll(states);
    sortAll(states);
    checkAll(states);

    byte *table = allocate(2*96*size(states) + 10000);
    resize(table, 2*96*size(states));
    compile(states, table);
    runTests(table, lines, states);

    char outfile[100];
    strcpy(outfile, args[1]);
    strcpy(outfile + strlen(outfile) - 4, ".bin");
    write(outfile, table);

    release(table);
    for (int i = 0; i < size(states); i++) release(states[i].patterns);
    release(states);
    for (int i = 0; i < size(rules); i++) release(rules[i].tokens);
    release(rules);
    release(lines);
    release(text);
}
