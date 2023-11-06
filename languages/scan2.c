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

// TODO: check tags.

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

// Split a line into tokens, adding a "+" for a rule with no tag.
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
    if (! hasTag) tokens[n++] = "+";
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
struct pattern { char *match; bool lookahead; char *tag; char *target; };
typedef struct pattern pattern;

// A state has a name, an array of patterns, and flags to say whether the state
// can occur at the start of a token, or can occur after the start. A visited
// flag helps check for cycles of states.
struct state { char *name; pattern *patterns; bool starts, resumes, visited; };
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
void convert(char *s, char *target, char *tag, pattern *p) {
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
        char *target = tokens[n-2];
//        char *tag = tokens[n-1];
        if (n < 4) error("incomplete rule on line %d", r->row);
        if (! islower(target[0])) {
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
    char *target = tokens[n-2];
    char *tag = tokens[n-1];
    if (findState(states, target) < 0) {
        error("unknown target state on line %d", r->row);
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
// specific patterns taking precedence.

// Single character strings \s !..~ \n for expanding ranges.
char *singles[96] = {
    " ","!","\"","#","$","%","&","'","(",")","*","+",",", "-",".","/",
    "0","1","2", "3","4","5","6","7","8","9",":",";","<", "=",">","?",
    "@","A","B", "C","D","E","F","G","H","I","J","K","L", "M","N","O",
    "P","Q","R", "S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
    "`","a","b", "c","d","e","f","g","h","i","j","k","l", "m","n","o",
    "p","q","r", "s","t","u","v","w","x","y","z","{","|", "}","~","\n"
};

// Find the single character string for a given character.
char *single(char ch) {
    if (ch == '\n') return singles[95];
    return singles[(int)ch - (int)' '];
}

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

// Expand a state's range into singles, and add them if not already handled.
void derange(state *base, range *r) {
    char *s = range.match;
    int n = size(base->patterns);
    for (int ch = s[0]; ch <= s[3]; ch++) {
        bool found = false;
        for (int i = 0; i < size(base->patterns); i++) {
            char *s = base->patterns[i].match;
            if (s[0] == ch && s[1] == '\0') { found = true; break; }
        }
        if (found) continue;
        base->patterns[n] = range;
        base->patterns[n].match = s;
        n++;
    }
    resize(base->patterns, n);
}



// Ranges are expanded by
// repeatedly finding a range with no subrange, and replacing it by
// one-character patterns for those characters not already handled.

int main() {
    char *text = readFile("c.txt");
    normalize(text);
    char **lines = splitLines(text);
    rule *rules = getRules(lines);
    printf("#rules: %d\n", size(rules));
    state *states = makeStates(rules);
    printf("#states: %d\n", size(states));
    fillStates(rules, states);
//    printf("%s [%s]\n", single('0'), single('\n'));
//    for (int i = 0; i < size(states); i++) {
//        printf("State %s: %d patterns\n", states[i].name, size(states[i].patterns));
//    }

    for (int i = 0; i < size(states); i++) release(states[i].patterns);
    release(states);
    for (int i = 0; i < size(rules); i++) release(rules[i].tokens);
    release(rules);
    release(lines);
    release(text);
}
