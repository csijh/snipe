// Snipe language compiler. Free and open source. See licence.txt.
// TODO consider binary again.
// TODO sort out lookahead past spaces.

// Compile a language description in .../rules.txt into a scanner table in
// .../table.txt. The program interpret.c can be used to test the table. A rule
// has a base state, patterns, a target state, and an optional tag at the
// beginning or end. A pattern may be a range such as 0..9 of single characters.
// A tag at the beginning of a rule indicates a lookahead rule, and a lack of
// patterns indicates a default lookahead rule.

// Each state must consistently be either a start state between tokens, or a
// continuation state within tokens. The default for a start state is to accept
// and suitably tag a space or newline, or to accept any other single character
// as an error token, and stay in the same state. The default for a continuation
// state is to terminate the current token as an error token and go to the
// initial state. If a lookahead rule has no tag, its target state must accept
// at least one character in any situation where the lookahead rule applies, to
// ensure progress.

// The resulting table has an entry for each state and pattern, with a tag and a
// target. Lookahead is handled by differentiating between normal patterns and
// lookahead patterns. The states are sorted with start states first, and the
// number of start states is limited to 32 so they can be cached by the scanner
// (in the tag bytes for spaces). The total number of states is limited to 128
// so a state index can be held in a char. The patterns are sorted, with
// longer ones before shorter ones, and normal patterns before lookahead
// patterns, so the next character in the text can be used to find the range of
// patterns to check. Each entry has a tag as an action, and a target state. The
// actions give a token type to the first character of a token, or indicate
// skipping the table entry, or looking ahead past spaces, or classify a text
// byte as a continuation byte of a character, a continuation character of a
// token, a space, or a newline.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// BIG is the fixed capacity of a list of pointers, including a terminating
// NULL. Increase as necessary. SMALL is the capacity of a state name or pattern
// string or tag name.
enum { BIG = 1000, SMALL = 128 };

// These are the tag symbols currently accepted in descriptions.
char const symbols[] = "()[]{}<>#/\\^$*'\"@=:?-";

// A tag is an upper case letter or one of the above symbols, which include MORE
// to indicate continuing the current token and BAD to indicate an error token.
// A tag can also be SKIP to mean ignore a table entry as not relevant to the
// current state (also used later in the scanner to tag bytes which continue a
// character), or GAP to tag a space as a gap between tokens or NEWLINE to tag a
// newline as a token.
enum { MORE = '-', BAD = '?', SKIP = '~', GAP = '_', NEWLINE = '.'};

// --------- Structures --------------------------------------------------------

// An action contains a tag and a target state, each stored in one byte.
struct action {
    char tag;
    unsigned char target;
};
typedef struct action action;

// A pattern has a name and a lookahead flag. Normal patterns and lookahead
// patterns are considered to be different. An index is added after sorting.
struct pattern {
    char name[SMALL];
    bool looks;
    int index;
};
typedef struct pattern pattern;

// A state has a name, a flag to indicate whether it is a start state or a
// continuation state, and a proof (i.e. the line number of the rule where the
// flag was established, or 0 if the flag hasn't been established yet). The
// rules which define the state are listed. An index is added after sorting. The
// actions for the state are added after that. The visiting and visited flags
// are used during progress checking.
struct state {
    char name[SMALL];
    bool starts;
    int proof;
    struct rule *rules[BIG];
    int index;
    action actions[BIG];
    bool visiting, visited;
};
typedef struct state state;

// A rule has a row (line number), a base state, patterns, a target state, and a
// tag. The default for a missing tag is MORE. There are flags to indicate if it
// is a lookahead rule (tag at start) or a default rule (no patterns).
struct rule {
    int row;
    state *base, *target;
    pattern *patterns[BIG];
    char tag;
    bool looks, defaults;
};
typedef struct rule rule;

// A language description has lists of rules, states and patterns.
struct language {
    rule *rules[BIG];
    state *states[BIG];
    pattern *patterns[BIG];
};
typedef struct language language;

// ----- Lists -----------------------------------------------------------------

// Crash with an error message and possibly a line number or string.
void crash(char *e, int row, char const *s) {
    fprintf(stderr, "Error:");
    fprintf(stderr, " %s", e);
    if (row > 0) fprintf(stderr, " on line %d", row);
    if (strlen(s) > 0) fprintf(stderr, " (%s)", s);
    fprintf(stderr, "\n");
    exit(1);
}

// Add an item to a list. Lists are NULL-terminated, to make them easy to
// initialize, and to iterate through without needing the length. (It would be
// nice to have generic functions, but in C the result is too ugly).
void addPattern(pattern *list[], pattern *p) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n++] = p;
    if (n >= BIG) crash("Too many patterns", 0, "");
    list[n] = NULL;
}

void addState(state *list[], state *s) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n++] =s;
    if (n >= BIG) crash("Too many states", 0, "");
    list[n] = NULL;
}

void addRule(rule *list[], rule *r) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n++] = r;
    if (n >= BIG) crash("Too many rules", 0, "");
    list[n] = NULL;
}

void addString(char *list[], char *s) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n++] = s;
    if (n >= BIG) crash("Too many strings", 0, "");
    list[n] = NULL;
}

// Find the length of a list.
int countPatterns(pattern *list[]) {
    int n = 0;
    while (list[n] != NULL) n++;
    return n;
}

int countStates(state *list[]) {
    int n = 0;
    while (list[n] != NULL) n++;
    return n;
}

int countRules(rule *list[]) {
    int n = 0;
    while (list[n] != NULL) n++;
    return n;
}

int countStrings(char *list[]) {
    int n = 0;
    while (list[n] != NULL) n++;
    return n;
}

// ----- Construction ----------------------------------------------------------

// Find a pattern by name and lookahead flag, or create a new one.
pattern *findPattern(language *lang, char *name, bool looks) {
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (strcmp(name,p->name) != 0) continue;
        if (p->looks == looks) return p;
    }
    pattern *p = malloc(sizeof(pattern));
    addPattern(lang->patterns, p);
    if (strlen(name) >= SMALL) crash("name too long", 0, name);
    strcpy(p->name, name);
    p->looks = looks;
    return p;
}

// Find a one-character pattern, or create a new one.
pattern *findPattern1(language *lang, char ch, bool looks) {
    char name[2] = "x";
    name[0] = ch;
    return findPattern(lang, name, looks);
}

// Find a state by name, or create a new one.
state *findState(language *lang, char *name) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        if (strcmp(name, lang->states[i]->name) == 0) return lang->states[i];
    }
    state *s = malloc(sizeof(state));
    addState(lang->states, s);
    if (strlen(name) >= SMALL) crash("name too long", 0, name);
    strcpy(s->name, name);
    s->proof = 0;
    s->rules[0] = NULL;
    return s;
}

// Find a tag character from a rule's token type.
char findTag(int row, char *name) {
    if ('A' <= name[0] && name[0] <= 'Z') return name[0];
    if (strlen(name) != 1) crash("bad token type", row, name);
    if (strchr(symbols, name[0]) == NULL) crash("bad token type", row, name);
    return name[0];
}

// Check whether a pattern string is a range of characters.
bool isRange(char *s) {
    return strlen(s) == 4 && s[1] == '.' && s[2] == '.';
}

// Add a token as a pattern for a rule, or a range x..y as multiple patterns.
void readPattern(language *lang, rule *r, char *token) {
    if (! isRange(token)) {
        pattern *p = findPattern(lang, token, r->looks);
        addPattern(r->patterns, p);
    }
    else for (int ch = token[0]; ch <= token[3]; ch++) {
        pattern *p = findPattern1(lang, ch, r->looks);
        addPattern(r->patterns, p);
    }
}

// Read a rule, if any, from the tokens on a given line.
void readRule(language *lang, int row, char *tokens[]) {
    if (tokens[0] == NULL) return;
    if (strlen(tokens[0]) > 1 && ! isalpha(tokens[0][0])) return;
    rule *r = malloc(sizeof(rule));
    addRule(lang->rules, r);
    r->row = row;
    r->looks = r->defaults = false;
    r->tag = GAP;
    int first = 0, last = countStrings(tokens) - 1;
    if (first >= last) crash("rule too short", row, "");
    if (! islower(tokens[first][0])) {
        r->tag = findTag(row, tokens[first]);
        r->looks = true;
        first++;
    }
    if (! islower(tokens[last][0])) {
        if (r->tag != GAP) crash("rule has a tag first and last", row, "");
        r->tag = findTag(row, tokens[last]);
        last--;
    }
    if (r->tag == GAP) r->tag = MORE;
    if (last == first + 1) r->looks = r->defaults = true;
    char *s = tokens[first];
    if (! islower(s[0])) crash("expecting state name", row, s);
    r->base = findState(lang, tokens[first++]);
    addRule(r->base->rules, r);
    s = tokens[last];
    if (! islower(s[0])) crash("expecting state name", row, s);
    r->target = findState(lang, tokens[last--]);
    r->patterns[0] = NULL;
    for (int i = first; i <= last; i++) {
        readPattern(lang, r, tokens[i]);
    }
}

// Create a language structure and add one-character patterns.
language *newLanguage() {
    language *lang = malloc(sizeof(language));
    lang->rules[0] = NULL;
    lang->states[0] = NULL;
    lang->patterns[0] = NULL;
    findPattern(lang, "\n", false);
    findPattern(lang, "\n", true);
    findPattern(lang, " ", false);
    findPattern(lang, " ", true);
    for (int ch = '!'; ch <= '~'; ch++) {
        findPattern1(lang, ch, false);
        findPattern1(lang, ch, true);
    }
    return lang;
}

void freeLanguage(language *lang) {
    for (int i = 0; lang->patterns[i] != NULL; i++) free(lang->patterns[i]);
    for (int i = 0; lang->states[i] != NULL; i++) free(lang->states[i]);
    for (int i = 0; lang->rules[i] != NULL; i++) free(lang->rules[i]);
    free(lang);
}

// ----- Reading in  -----------------------------------------------------------

// Read a text file as a string, adding a final newline if necessary, and a null
// terminator. Use binary mode, so that the file size equals the bytes read in.
char *readFile(char const *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("can't read file", 0, path);
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("can't find file size", 0, path);
    char *text = malloc(size+2);
    int n = fread(text, 1, size, file);
    if (n != size) crash("can't read file", 0, path);
    if (n > 0 && text[n - 1] != '\n') text[n++] = '\n';
    text[n] = '\0';
    fclose(file);
    return text;
}

// Validate a line. Check it is ASCII only. Convert '\t' or '\r' to a space. Ban
// other control characters.
void validateLine(int row, char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        unsigned char ch = line[i];
        if (ch == '\t' || ch == '\r') line[i] = ' ';
        else if (ch >= 128) crash("non-ASCII character", row, "");
        else if (ch < ' ' || ch > '~') crash("control character", row, "");
    }
}

// Split the text into a list of lines, replacing each newline by a null.
void splitLines(char *text, char *lines[]) {
    int p = 0, row = 1;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i]= '\0';
        validateLine(row, &text[p]);
        addString(lines, &text[p]);
        p = i + 1;
        row++;
    }
}

// Split a line into a list of tokens.
void splitTokens(language *lang, int row, char *line, char *tokens[]) {
    int start = 0, end = 0, len = strlen(line);
    while (line[start] == ' ') start++;
    while (start < len) {
        end = start;
        while (line[end] != ' ' && line[end] != '\0') end++;
        line[end] = '\0';
        char *pattern = &line[start];
        addString(tokens, pattern);
        start = end + 1;
        while (start < len && line[start] == ' ') start++;
    }
}

// Convert the list of lines into rules.
void readRules(language *lang, char *lines[]) {
    char **tokens = malloc(BIG * sizeof(char *));
    tokens[0] = NULL;
    for (int i = 0; lines[i] != NULL; i++) {
        char *line = lines[i];
        splitTokens(lang, i+1, line, tokens);
        readRule(lang, i+1, tokens);
        tokens[0] = NULL;
    }
    free(tokens);
}

// Read a language description from text.
language *readLanguage(char *text) {
    char **lines = malloc(BIG * sizeof(char *));
    lines[0] = NULL;
    splitLines(text, lines);
    language *lang = newLanguage();
    readRules(lang, lines);
    free(lines);
    return lang;
}

// ---------- Consistency ------------------------------------------------------

// Check that every state mentioned has at least one rule.
void checkDefined(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (countRules(s->rules) > 0) continue;
        crash("undefined state", 0, s->name);
    }
}

// Set state s to be a start state or continuation state according to the flag.
// Give error message if already found out to be the opposite.
void setStart(state *s, bool starts, int row) {
    if (s->proof > 0 && s->starts != starts) {
        fprintf(stderr, "Error: %s is a ", s->name);
        if (s->starts) fprintf(stderr, "start");
        else fprintf(stderr, "continuation");
        fprintf(stderr, " state because of line %d\n", s->proof);
        if (starts) fprintf(stderr, "but can occur between tokens");
        else fprintf(stderr, "but can occur within a token");
        fprintf(stderr, " because of line %d\n", row);
        exit(1);
    }
    s->starts = starts;
    s->proof = row;
}

// Find start states and continuation states, and check consistency.
// Any rule with a token type has a start state as its target.
// A normal rule with no token type has a continuation state as its target.
// A lookahead rule with a token type has a continuation state as its base.
void findStartStates(language *lang) {
    state *s0 = lang->rules[0]->base;
    s0->starts = true;
    s0->proof = lang->rules[0]->row;
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (r->tag != MORE) setStart(r->target, true, r->row);
        if (! r->looks && r->tag == MORE) setStart(r->target, false, r->row);
        if (r->looks && r->tag != MORE) setStart(r->base, false, r->row);
    }
}

// Check that an explicit lookahead rule in a continuation state has a token
// type (because there may be a space which must terminate the current token.)
void checkLookaheads(language *lang) {
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (! r->base->starts && r->looks && ! r->defaults && r->tag == MORE) {
            fprintf(stderr, "Error in rule on line %d\n", r->row);
            fprintf(stderr, "an explicit default rule in a ");
            fprintf(stderr, "continuation state must end the current token\n");
            exit(1);
        }
    }
}

// Check that a lookahead or default rule with no token type (i.e. a jump rule)
// has base and target states which are both start states or both continuations.
void checkTransfers(language *lang) {
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (! (r->looks || r->defaults) || r->tag != MORE) continue;
        if (r->base->starts == r->target->starts) continue;
        fprintf(stderr, "Error in rule on line %d\n", r->row);
        if (r->base->starts) {
            fprintf(stderr, "%s is a start state ", r->base->name);
            fprintf(stderr, "(line %d)\n", r->base->proof);
            fprintf(stderr, "but %s is a continuation state ", r->target->name);
            fprintf(stderr, "(line %d)\n", r->target->proof);
        }
        else {
            fprintf(stderr, "%s is a continuation state ", r->base->name);
            fprintf(stderr, "(line %d)\n", r->base->proof);
            fprintf(stderr, "but %s is a start state ", r->target->name);
            fprintf(stderr, "(line %d)\n", r->target->proof);
        }
        exit(1);
    }
}

// For each state, report a rule that comes after a default as inaccessible.
void checkAccessible(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        bool hasDefault = false;
        for (int j = 0; s->rules[j] != NULL; j++) {
            rule *r = s->rules[j];
            if (countPatterns(r->patterns) == 0) hasDefault = true;
            else if (hasDefault) crash("inaccessible rule", r->row, "");
        }
    }
}

// ----- Sorting --------------------------------------------------------------

// Sort the states so that the start states come before the continuation states.
// Limit the number of start states to 32, and the total number to 128. Add
// indexes.
void sortStates(state *states[]) {
    for (int i = 1; states[i] != NULL; i++) {
        state *s = states[i];
        int j = i - 1;
        if (! s->starts) continue;
        while (j >= 0 && ! states[j]->starts) {
            states[j + 1] = states[j];
            j--;
        }
        states[j + 1] = s;
    }
    for (int i = 0; states[i] != NULL; i++) {
        state *s = states[i];
        if (s->starts && i >= 32) crash("more than 32 start states", 0, "");
        if (i >= 128) crash("more than 128 states", 0, "");
        s->index = i;
    }
}

// Compare two patterns in ASCII order, except prefer longer strings and
// non-lookaheads.
int compare(pattern *p, pattern *q) {
    char *s = p->name, *t = q->name;
    for (int i = 0; ; i++) {
        if (s[i] == '\0' && t[i] == '\0') break;
        if (s[i] == '\0') return 1;
        if (t[i] == '\0') return -1;
        if (s[i] < t[i]) return -1;
        if (s[i] > t[i]) return 1;
    }
    if (! p->looks && q->looks) return -1;
    if (p->looks && ! q->looks) return 1;
    return 0;
}

// Sort the patterns. Add indexes.
void sortPatterns(pattern *patterns[]) {
    for (int i = 1; patterns[i] != NULL; i++) {
        pattern *p = patterns[i];
        int j = i - 1;
        while (j >= 0 && compare(patterns[j], p) > 0) {
            patterns[j + 1] = patterns[j];
            j--;
        }
        patterns[j + 1] = p;
    }
    for (int i = 0; patterns[i] != NULL; i++) patterns[i]->index = i;
}

// ----- Actions ---------------------------------------------------------------

// Fill in all states with SKIP actions.
void fillSkipActions(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        for (int j = 0; lang->patterns[j] != NULL; j++) {
            s->actions[j].tag = SKIP;
            s->actions[j].target = 0;
        }
    }
}

// Fill in the implicit defaults for a start state, for all match characters not
// already handled.
void fillMatchActions(language *lang, state *s) {
    for (char ch = '\n'; ch <= '~'; ch++) {
        if ('\n' < ch && ch < ' ') continue;
        pattern *p = findPattern1(lang, ch, false);
        int n = p->index;
        if (s->actions[n].tag != SKIP) continue;
        if (ch == '\n') s->actions[n].tag = NEWLINE;
        else if (ch == ' ') s->actions[n].tag = GAP;
        else s->actions[n].tag = BAD;
        s->actions[n].target = s->index;
    }
}

// Fill in actions for an explicit default rule, or the implicit defaults for a
// continuation state, for all lookahead characters not already handled.
void fillDefaultActions(language *lang, state *base, state *target, char tag) {
    for (char ch = '\n'; ch <= '~'; ch++) {
        if ('\n' < ch && ch < ' ') continue;
        pattern *p = findPattern1(lang, ch, true);
        int n = p->index;
        if (base->actions[n].tag != SKIP) continue;
        base->actions[n].tag = tag;
        base->actions[n].target = target->index;
    }
}

// Fill in the actions for a rule, for patterns not already handled.
void fillRuleActions(language *lang, rule *r) {
    state *s = r->base, *t = r->target;
    if (r->defaults) fillDefaultActions(lang, s, t, r->tag);
    else for (int i = 0; r->patterns[i] != NULL; i++) {
        pattern *p = r->patterns[i];
        int n = p->index;
        if (s->actions[n].tag != SKIP) continue;
        s->actions[n].tag = r->tag;
        s->actions[n].target = t->index;
    }
}

// Fill in all explicit actions, and implicit defaults.
void fillActions(language *lang) {
    fillSkipActions(lang);
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        fillRuleActions(lang, r);
    }
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (s->starts) fillMatchActions(lang, s);
        else fillDefaultActions(lang, s, lang->states[0], BAD);
    }
}

// ---------- Progress ---------------------------------------------------------

// Before doing a search to check progress for a character, set the visit flags
// to false.
void startSearch(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        s->visiting = s->visited = false;
    }
}

// Find the index of the first pattern starting with a given character.
int firstPattern(language *lang, char ch) {
    int i;
    for (i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (p->name[0] >= ch) return i;
    }
    return i;
}

// Visit a state during a depth first search for a loop with no progress when
// the given character is next in the input. A lookahead pattern starting with
// the character indicates a progress-free jump to another state. A match or
// lookahead pattern which is that single character indicates no more possible
// jumps from the state. Return true for success, false for a loop.
bool visit(language *lang, state *s, char ch) {
    if (s->visited) return true;
    if (s->visiting) return false;
    s->visiting = true;
    int n = firstPattern(lang, ch);
    for (int i = n; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (p->name[0] != ch) break;
        if (p->looks) {
            bool ok = visit(lang, lang->states[s->actions[i].target], ch);
            if (! ok) return false;
        }
        if (strlen(p->name) == 1) break;
    }
    s->visited = true;
    return true;
}

// Report a progress-free loop of states when ch is next in the input.
void reportLoop(language *lang, char ch) {
    fprintf(stderr, "Error: possible infinite loop with no progress\n");
    fprintf(stderr, "when character '");
    if (ch == '\n') fprintf(stderr, "\\n");
    else fprintf(stderr, "%c", ch);
    fprintf(stderr, "' is next in the input.\n");
    fprintf(stderr, "The states involved are:");
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (! s->visiting) continue;
        fprintf(stderr, " %s", s->name);
    }
    fprintf(stderr, "\n");
    exit(1);
}

// For each character, carry out a depth first search for a loop of
// progress-free jumps between states when that character is next in the input.
// Report any loop found.
void checkProgress(language *lang) {
    for (int ch = '\n'; ch <= '~'; ch++) {
        if (ch > '\n' && ch < ' ') continue;
        startSearch(lang);
        for (int i = 0; lang->states[i] != NULL; i++) {
            state *s = lang->states[i];
            bool ok = visit(lang, s, ch);
            if (! ok) reportLoop(lang, ch);
        }
    }
}

// ---------- Building and printing --------------------------------------------

// Read a language, analyse it, and generate actions.
language *buildLanguage(char *text) {
    language *lang = readLanguage(text);
    checkDefined(lang);
    findStartStates(lang);
    checkLookaheads(lang);
    checkTransfers(lang);
    checkAccessible(lang);
    sortStates(lang->states);
    sortPatterns(lang->patterns);
    fillActions(lang);
    checkProgress(lang);
    return lang;
}

// Write out the index number of a state in hex as a digit followed by a space
// or two digits.
void writeIndex(FILE *fp, int index) {
    if (index < 16) fprintf(fp, "%1x ", index);
    else fprintf(fp, "%2x", index);
}

// Write out a row of state names, then a row of state indexes (as column
// headers) then a row of actions per pattern. Print the pattern on the end of
// the row, with prefix '-' for matching or '|' for lookahead.
void writeTable(language *lang, char const *path) {
    FILE *fp = fopen(path, "w");
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (i > 0) fprintf(fp, " ");
        fprintf(fp, "%s", s->name);
    }
    fprintf(fp, "\n");
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        for (int j = 0; lang->states[j] != NULL; j++) {
            state *s = lang->states[j];
            char tag = s->actions[i].tag;
            int target = s->actions[i].target;
            fprintf(fp, "%c", tag);
            writeIndex(fp, target);
            fprintf(fp, "  ");
        }
        if (p->looks) fprintf(fp, "|");
        else fprintf(fp, "-");
        if (p->name[0] == '\n') fprintf(fp, "nl\n");
        else if (p->name[0] == ' ') fprintf(fp, "sp\n");
        else fprintf(fp, "%s\n", p->name);
    }
    fclose(fp);
}

// Write out a binary file containing the names of the states, each name being a
// string preceded by a length byte, then a zero byte, then the pattern strings,
// each preceded by a length byte with the top bit set for a lookahead pattern,
// then a zero byte, then the array of actions for each state.
void writeTable2(language *lang, char const *path) {
    FILE *fp = fopen(path, "wb");
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        int n = strlen(s->name);
        fprintf(fp, "%c%s%c", n, s->name, 0);
    }
    fprintf(fp, "%c", 0);
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        int n = strlen(p->name);
        if (p->looks) n = n | 0x80;
        fprintf(fp, "%c%s%c", n, p->name, 0);
    }
    fprintf(fp, "%c", 0);
    int np = countPatterns(lang->patterns);
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        fwrite(s->actions, 2, np, fp);
    }
    fclose(fp);
}

// ----- Testing ---------------------------------------------------------------

// Check that, in the named example, the given test succeeds. The test
// represents an action, i.e. an entry in the table to be generated. It is
// expressed roughly in the original rule format (base state name, single
// pattern or \n or \s, target state name, single character tag at either end).
void checkAction(language *lang, char *name, char *test) {
    char text[100];
    strcpy(text, test);
    char *tokens[5] = { NULL };
    splitTokens(lang, 0, text, tokens);
    state *s, *t;
    pattern *p;
    char tag;
    if (islower(tokens[0][0])) {
        s = findState(lang, tokens[0]);
        if (strcmp(tokens[1],"\\n") == 0) tokens[1] = "\n";
        if (strcmp(tokens[1],"\\s") == 0) tokens[1] = " ";
        p = findPattern(lang, tokens[1], false);
        t = findState(lang, tokens[2]);
        tag = tokens[3][0];
    }
    else {
        s = findState(lang, tokens[1]);
        if (strcmp(tokens[2],"\\n") == 0) tokens[2] = "\n";
        if (strcmp(tokens[2],"\\s") == 0) tokens[2] = " ";
        p = findPattern(lang, tokens[2], true);
        t = findState(lang, tokens[3]);
        tag = tokens[0][0];
    }
    char actionTag = s->actions[p->index].tag;
    int actionTarget = s->actions[p->index].target;
    if (actionTag == tag && actionTarget == t->index) return;
    fprintf(stderr, "Test failed: %s: %s (%c %s)\n",
        name, test, actionTag, lang->states[actionTarget]->name);
    exit(1);
}

// Run the tests in an example.
void runExample(char *name, char *eg[]) {
    char text[1000];
    strcpy(text, eg[0]);
    language *lang = buildLanguage(text);
    for (int i = 1; eg[i] != NULL; i++) checkAction(lang, name, eg[i]);
    freeLanguage(lang);
}

// Examples from help/languages.xhtml. Each example has a string forming a
// language description (made of concatenated lines), then strings which test
// some generated table entries, then a NULL. Each test checks an entry in the
// generated table, expressed roughly in the original rule format (state name,
// single pattern or \n or \s, state name, single character tag at either end).

// One basic illustrative rule.
char *eg1[] = {
    "start == != start OP\n",
    // ----------------------
    "start == start O",
    "start != start O", NULL
};

// Rule with no tag, continuing the token. Range pattern.
char *eg2[] = {
    "start 0..9 number\n"
    "number 0..9 start VALUE\n",
    //--------------------------
    "start 0 number -",
    "start 5 number -",
    "start 9 number -",
    "number 0 start V",
    "number 5 start V",
    "number 9 start V", NULL
};

// Symbol as token type, i.e. ? for error token.
char *eg3[] = {
    "start \\ escape\n"
    "escape n start ?\n",
    //-------------------
    "start \\ escape -",
    "escape n start ?", NULL
};

// Multiple rules, whichever matches the next input is used.
char *eg4[] = {
    "start == != start OP\n"
    "start a..z A..Z id\n"
    "id a..z A..Z start ID\n",
    //------------------------
    "start == start O",
    "start x id -",
    "id x start I", NULL
};

// Longer pattern takes precedence (= is a prefix of ==). (Can't test here
// whether the patterns are ordered so that == is before =)
char *eg5[] = {
    "start = start SIGN\n"
    "start == != start OP\n",
    //-----------------------
    "start = start S",
    "start == start O", NULL
};

// Earlier rule for same pattern takes precedence (> is matched by last rule).
char *eg6[] = {
    "start < filename\n"
    "filename > start =\n"
    "filename !..~ filename\n",
    //-------------------------
    "start < filename -",
    "filename > start =",
    "filename ! filename -", NULL
};

// A lookahead rule allows a token's type to be affected by the next token.
char *eg7[] = {
    "start a..z A..Z id\n"
    "id a..z A..Z 0..9 id\n"
    "FUN id ( start\n"
    "id start ID\n",
    //--------------
    "start f id -",
    "FUN id ( start",
    "ID id ; start", NULL
};

// A lookahead rule can be a continuation. It is a jump to another state.
char *eg8[] = {
    "start a start ID\n"
    "- start . start2\n"
    "start2 . start2 DOT\n"
    "start2 start\n",
    //-------------------
    "- start . start2", NULL
};

// Identifier may start with keyword. A default rule ("matching the empty
// string") is turned into a lookahead rule for each unhandled character.
char *eg9[] = {
    "start a..z A..Z id\n"
    "start if else for while key\n"
    "key a..z A..Z 0..9 id\n"
    "key start KEY\n"
    "id a..z A..Z 0..9 id\n"
    "id start ID\n",
    //--------------
    "start f id -",
    "start for key -",
    "key m id -",
    "KEY key ; start", NULL
};

// A default rule with no tag is an unconditional jump.
char *eg10[] = {
    "start #include inclusion KEY\n"
    "inclusion < filename\n"
    "inclusion start\n"
    "filename > start QUOTED\n"
    "filename !..~ filename\n"
    "filename start ?\n",
    //--------------
    "start #include inclusion K",
    "inclusion < filename -",
    "- inclusion ! start",
    "- inclusion x start",
    "- inclusion ~ start",
    "- inclusion \\n start",
    "- inclusion \\s start", NULL
};

// Can have multiple start states. Check that the default is to accept any
// single character as an error token and stay in the same state.
char *eg11[] = {
    "start # hash KEY\n"
    "hash include start RESERVED\n"
    "html <% java <\n"
    "java %> html >\n",
    //--------------
    "start # hash K",
    "hash include start R",
    "hash x hash ?",
    "hash i hash ?",
    "hash \\n hash .",
    "hash \\s hash _",
    "html <% java <",
    "html x html ?",
    "java %> html >",
    "java x java ?", NULL
};

// Corrected.
char *eg12[] = {
    "start . dot\n"
    "dot 0..9 start NUM\n"
    "SIGN dot a..z A..Z prop\n"
    "prop a..z A..Z prop2\n"
    "prop start\n"
    "prop2 a..z A..Z 0..9 prop2\n"
    "prop2 start PROPERTY\n",
    //-----------------------
    "dot 0 start N",
    "S dot x prop",
    "prop x prop2 -",
    "prop2 x prop2 -",
    "P prop2 ; start", NULL
};

// CAUSES ERROR. The number state is not defined.
char *eg13[] = {
    "start . dot\n"
    "dot 0..9 number\n"
    "SIGN dot a..z A..Z prop\n"
    "prop a..z A..Z 0..9 prop\n"
    "prop start PROPERTY\n",
    //----------------------
    "dot 0 number -", NULL
};

// CAUSES ERROR. The prop state is a start state because
// of line 3, and continuation state because of lines 4/5.
char *eg14[] = {
    "start . dot\n"
    "dot 0..9 start NUM\n"
    "SIGN dot a..z A..Z prop\n"
    "prop a..z A..Z 0..9 prop\n"
    "prop start PROPERTY\n",
    //----------------------
    "dot 0 start N", NULL
};

// CAUSES ERROR. An explicit lookahead rule in a continuation state must end the
// current token.
char *eg15[] = {
    "start a id\n"
    "- id ( id2\n"
    "id2 a id2\n"
    "id2 start ID\n",
    //----------------------
    "- id ( id2", NULL
};

// CAUSES ERROR. An explicit lookahead rule in a start state must not end the
// current token.
char *eg16[] = {
    "DOT start . start2\n"
    "start2 start\n",
    //----------------------
    "DOT start . start2", NULL
};

// Run all the tests. Keep the last few commented out during normal operation
// because they test error messages.
void runTests() {
    runExample("eg1", eg1);
    runExample("eg2", eg2);
    runExample("eg3", eg3);
    runExample("eg4", eg4);
    runExample("eg5", eg5);
    runExample("eg6", eg6);
    runExample("eg7", eg7);
    runExample("eg8", eg8);
    runExample("eg9", eg9);
    runExample("eg10", eg10);
    runExample("eg11", eg11);
    runExample("eg12", eg12);
//    runExample("eg13", eg13);
//    runExample("eg14", eg14);
//    runExample("eg15", eg15);
//    runExample("eg16", eg16);
}

int main(int n, char const *args[n]) {
    if (n != 2) crash("Use: ./compile language", 0, "");
    char path[100];
    runTests();
    sprintf(path, "%s/rules.txt", args[1]);
    char *text = readFile(path);
    language *lang = buildLanguage(text);
    sprintf(path, "%s/table.txt", args[1]);
    writeTable(lang, path);
    sprintf(path, "%s/table.bin", args[1]);
    writeTable2(lang, path);
    freeLanguage(lang);
    free(text);
    return 0;
}
