// Snipe language compiler. Free and open source. See licence.txt.

// Compile a language description in .../rules.txt into a scanner table in
// .../table.txt. The program interpret.c can be used to test the table. A rule
// has a base state, patterns, a target state, and an optional tag at the
// beginning or end. A pattern may be a range of single characters a..z. A tag
// at the beginning of a rule indicates a lookahead rule, and a lack of patterns
// indicates a default lookahead rule.

// Each state must consistently be either a start state between tokens, or a
// continuation state within tokens. The default for a start state is to accept
// any single character as an error token, or accept and tag a space or newline,
// and stay in the same state. The default for a continuation state is to
// terminate the current token as an error token and go to the initial state. If
// a lookahead rule has no tag, its target state must accept at least one
// character in any situation where the lookahead rule applies, to ensure
// progress. The patterns in lookahead rules are flagged as as different from
// ordinary patterns.

// The resulting table has an entry for each state and pattern. The states are
// sorted with start states first, and the number of start states is limited to
// 32 so they can be cached by the scanner, in the tags for spaces. The patterns
// are sorted, with longer ones before shorter ones, and normal patterns before
// lookahead patterns, so the next character in the text can be used to find the
// range of patterns to check. Each entry has a tag as an action and a target
// state. The actions give a token type to the first character of a token, mark
// a continuation character of a token or continuation byte of a character, flag
// a space as a gap between tokens or a newline as a one-character token, or
// indicate skipping the entry or looking ahead past spaces.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// LIST is the fixed capacity of a list of pointers, including a NULL
// terminator. NAME is the capacity of a state name or pattern string or tag.
enum { LIST = 1000, NAME = 100 };

// State indexes and tags are stored in bytes.
typedef unsigned char byte;

// These are the tag symbols currently accepted in descriptions.
char const symbols[] = "()[]{}<>#/\\^$*'\"@=:?-";

// A tag is one of the above symbols, or an upper case letter, representing a
// token type, including MORE to mean continue the current token (also used in
// the scanner to tag characters which continue a token). It can also be SKIP to
// mean ignore a table entry as not relevant to the current state (also used
// later in the scanner to tag bytes which continue a character), or GAP to tag
// a space as a gap between tokens or NEWLINE to tag a newline as a token.
enum { SKIP = '~', MORE = '-', GAP = '_', NEWLINE = '.'};

// --------- Structures --------------------------------------------------------

// An action contains a tag and a target state. A token type tag means terminate
// the current token with that type. Tag SKIP means ignore the entry as not
// relevant to the current state, and MORE means continue the current token.
struct action {
    byte tag, target;
};
typedef struct action action;

// A pattern has a name and a lookahead flag. Normal patterns and lookahead
// patterns are considered to be different. An index is added after sorting.
struct pattern {
    char name[NAME];
    bool look;
    int index;
};
typedef struct pattern pattern;

// A state has a name, a flag to indicate whether it is a start state or a
// continuation state, and a proof, i.e. the line number of the rule where the
// flag was established, or 0 if the flag hasn't been established yet. The rules
// are the ones with the state as their base, which define the behaviour of the
// state. An index is added after sorting. The actions for the state are added
// after that.
struct state {
    char name[NAME];
    bool start;
    int proof;
    struct rule *rules[LIST];
    int index;
    action actions[LIST];
};
typedef struct state state;

// A rule has a row, i.e. line number, a lookahead flag, a base state, a list of
// patterns, a target state, and a tag. If the rule has no tag, the default tag
// is MORE. If the line number is zero, the rule is an automatically added
// default.
struct rule {
    int row;
    bool look;
    state *base, *target;
    pattern *patterns[LIST];
    byte tag;
};
typedef struct rule rule;

// A language description has lists of rules, states and patterns.
struct language {
    rule *rules[LIST];
    state *states[LIST];
    pattern *patterns[LIST];
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
// nice to have a single generic function, but in C the result is too ugly).
void addPattern(pattern *list[], pattern *p) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n++] = p;
    if (n >= LIST) crash("Too many patterns", 0, "");
    list[n] = NULL;
}

void addState(state *list[], state *s) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n++] =s;
    if (n >= LIST) crash("Too many states", 0, "");
    list[n] = NULL;
}

void addRule(rule *list[], rule *r) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n++] = r;
    if (n >= LIST) crash("Too many rules", 0, "");
    list[n] = NULL;
}

void addString(char *list[], char *s) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n++] = s;
    if (n >= LIST) crash("Too many strings", 0, "");
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

// Find a state by name, or create a new one.
state *findState(language *lang, char *name) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        if (strcmp(name, lang->states[i]->name) == 0) return lang->states[i];
    }
    state *s = malloc(sizeof(state));
    addState(lang->states, s);
    strcpy(s->name, name);
    s->proof = 0;
    s->rules[0] = NULL;
    return s;
}

// Find a pattern by name, or create a new one.
pattern *findPattern(language *lang, bool look, char *name) {
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (p->look != look) continue;
        if (strcmp(name,p->name) == 0) return p;
    }
    pattern *p = malloc(sizeof(pattern));
    addPattern(lang->patterns, p);
    strcpy(p->name, name);
    p->look = look;
    return p;
}

// Find a tag character from a rule's token type. Allow symbols _ . only for
// added default rules.
byte findTag(int row, char *name) {
    if ('A' <= name[0] && name[0] <= 'Z') return name[0];
    if (strlen(name) != 1) crash("bad token type", row, name);
    if (row != 0 && strchr(symbols, name[0]) == NULL) {
        crash("bad token type", row, name);
    }
    return name[0];
}

// Check whether a pattern string is a range of characters.
bool isRange(char *s) {
    return strlen(s) == 4 && s[1] == '.' && s[2] == '.';
}

// Add a token as a pattern, or a range x..y as multiple patterns.
void readPattern(language *lang, pattern *patterns[], bool look, char *token) {
    if (! isRange(token)) {
        pattern *p = findPattern(lang, look, token);
        addPattern(patterns, p);
        return;
    }
    char start = token[0], end = token[3];
    char name[2] = "x";
    for (int ch = start; ch <= end; ch++) {
        name[0] = ch;
        pattern *p = findPattern(lang, look, name);
        addPattern(patterns, p);
    }
}


// Read a rule, if any, from the tokens on a given line.
void readRule(language *lang, int row, char *tokens[]) {
    if (tokens[0] == NULL) return;
    if (strlen(tokens[0]) > 1 && ! isalpha(tokens[0][0])) return;
    rule *r = malloc(sizeof(rule));
    addRule(lang->rules, r);
    r->row = row;
    r->look = false;
    r->tag = GAP;
    int first = 0, last = countStrings(tokens) - 1;
    if (! islower(tokens[first][0])) {
        r->tag = findTag(row, tokens[first]);
        r->look = true;
        first++;
    }
    if (! islower(tokens[last][0])) {
        if (r->tag != GAP) crash("rule has tag first and last", row, "");
        r->tag = findTag(row, tokens[last]);
        last--;
    }
    if (r->tag == GAP) {
        r->tag = MORE;
        if (last == first + 1) r->look = true;
    }
    if (first >= last) crash("rule too short", row, "");
    char *s = tokens[first];
    if (! islower(s[0])) crash("expecting state name", row, s);
    r->base = findState(lang, tokens[first++]);
    addRule(r->base->rules, r);
    s = tokens[last];
    if (! islower(s[0])) crash("expecting state name", row, s);
    r->target = findState(lang, tokens[last--]);
    r->patterns[0] = NULL;
    for (int i = first; i <= last; i++) {
        readPattern(lang, r->patterns, r->look, tokens[i]);
    }
}

// Create a language structure and add one-character patterns.
language *newLanguage() {
    language *lang = malloc(sizeof(language));
    lang->rules[0] = NULL;
    lang->states[0] = NULL;
    lang->patterns[0] = NULL;
    findPattern(lang, false, "\n");
    findPattern(lang, true, "\n");
    findPattern(lang, false, " ");
    findPattern(lang, true, " ");
    char name[2] = "x";
    for (int ch = '!'; ch <= '~'; ch++) {
        name[0] = ch;
        findPattern(lang, false, name);
        findPattern(lang, true, name);
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
    char **tokens = malloc(LIST * sizeof(char *));
    tokens[0] = NULL;
    for (int i = 0; lines[i] != NULL; i++) {
        char *line = lines[i];
        splitTokens(lang, i+1, line, tokens);
        readRule(lang, i+1, tokens);
        tokens[0] = NULL;
    }
    free(tokens);
}

// Read a language description from a file.
language *readLanguage(char const *path) {
    char *text = readFile(path);
    char **lines = malloc(LIST * sizeof(char *));
    lines[0] = NULL;
    splitLines(text, lines);
    language *lang = newLanguage();
    readRules(lang, lines);
    free(lines);
    free(text);
    return lang;
}

// ---------- Consistency ------------------------------------------------------

// Set state s to be a start state or continuation state according to the flag.
// Give error message if already found out to be the opposite.
void setStart(state *s, bool start, int row) {
    if (s->proof > 0 && s->start != start) {
        fprintf(stderr, "Error: %s is a ", s->name);
        if (s->start) fprintf(stderr, "start");
        else fprintf(stderr, "continuation");
        fprintf(stderr, " state because of line %d\n", s->proof);
        if (start) fprintf(stderr, "but can occur between tokens");
        else fprintf(stderr, "but can occur within a token");
        fprintf(stderr, " because of line %d\n", row);
        exit(1);
    }
    s->start = start;
    s->proof = row;
}

// Find start states and continuation states, and check consistency.
// Any rule with a token type has a start state as its target.
// A normal rule with no token type has a continuation state as its target.
// A lookahead rule with a token type has a continuation state as its base.
void findStartStates(language *lang) {
    state *s0 = lang->rules[0]->base;
    s0->start = true;
    s0->proof = lang->rules[0]->row;
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (r->tag != MORE) {
            setStart(r->target, true, r->row);
        }
        if (! r->look && r->tag == MORE) {
            setStart(r->target, false, r->row);
        }
        if (r->look && r->tag != MORE) {
            setStart(r->base, false, r->row);
        }
    }
}

// Check that a lookahead rule with no token type (i.e. a jump rule) has
// base and target states which are both start states or both continuations.
void checkTransfers(language *lang) {
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (! r->look || r->tag != MORE) continue;
        if (r->base->start == r->target->start) continue;
        fprintf(stderr, "Error in rule on line %d\n", r->row);
        fprintf(stderr, "%s is a ", r->base->name);
        if (r->base->start) fprintf(stderr, "start");
        else fprintf(stderr, "continuation");
        fprintf(stderr, " state but %s isn't\n", r->target->name);
        exit(1);
    }
}

// ---------- Defaults ---------------------------------------------------------

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

// Add defaults for a start state, if necessary. (If the state has a default
// rule, it must be a jump to another state.) The defaults tag any one printable
// character as an error token, a space as a GAP, and a newline as a NEWLINE.
void addStartDefaults(language *lang, state *s) {
    int n = countRules(s->rules);
    if (n == 0) crash("undefined state", 0, s->name);
    if (countPatterns(s->rules[n-1]->patterns) == 0) return;
    char *chTokens[] = { s->name, "!..~", s->name, "?", NULL };
    readRule(lang, 0, chTokens);
    char gap[] = { GAP, '\0' };
    char *spTokens[] = { s->name, " ", s->name, gap, NULL };
    readRule(lang, 0, spTokens);
    char nl[] = { NEWLINE, '\0' };
    char *nlTokens[] = { s->name, "\n", s->name, nl, NULL };
    readRule(lang, 0, nlTokens);
}

// Add defaults for a continuation state. Add a lookahead rule for any character
// not already handled. If the state has a default lookahead rule with a target
// type, that determines the handling of spaces and newlines.
void addContinuationDefaults(language *lang, state *s) {
    int n = countRules(s->rules);
    if (n == 0) crash("undefined state", 0, s->name);
    if (countPatterns(s->rules[n-1]->patterns) > 0) {
        char *chTokens[] = { "?", s->name, "!..~", s->name, NULL };
        readRule(lang, 0, chTokens);
    }
    char gap[] = { GAP, '\0' };
    char *spTokens[] = { gap, s->name, " ", s->name, NULL };
    readRule(lang, 0, spTokens);
    char nl[] = { NEWLINE, '\0' };
    char *nlTokens[] = { nl, s->name, "\n", s->name, NULL };
    readRule(lang, 0, nlTokens);
}

void addDefaults(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (s->start) addStartDefaults(lang, s);
        else addContinuationDefaults(lang, s);
    }
}

// ---------- Progress ---------------------------------------------------------

// Check if string s is a prefix of string t.
bool prefix(char const *s, char const *t) {
    return strncmp(s, t, strlen(s)) == 0;
}

// For a given state, check whether a particular pattern, or a non-empty prefix
// of it, is explicitly accepted, so that progress is made.
bool checkAccept(language *lang, state *s, char *p) {
    for (int i = 0; s->rules[i] != NULL; i++) {
        rule *r = s->rules[i];
        if (r->look) continue;
        for (int i = 0; r->patterns[i] != NULL; i++) {
            if (prefix(r->patterns[i]->name, p)) return true;
        }
    }
    return true;
}

// For every explicit pattern in a lookahead rule which has no token type, make
// sure the pattern or a prefix of it is matched by the target state.
void checkPatternProgress(language *lang) {
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (! r->look) continue;
        if (r->tag != MORE) continue;
        state *t = r->target;
        for (int j = 0; r->patterns[j] != NULL; j++) {
            checkAccept(lang, t, r->patterns[j]->name);
        }
    }
}

// For a given state, check whether a particular one-character pattern is
// explicitly handled (as a match or a lookahead).
bool checkHandle(language *lang, state *s, char *p) {
    for (int i = 0; s->rules[i] != NULL; i++) {
        rule *r = s->rules[i];
        for (int i = 0; r->patterns[i] != NULL; i++) {
            if (strlen(r->patterns[i]->name) != 1) continue;
            if (r->patterns[i]->name[0] == p[0]) return true;
        }
    }
    return true;
}

// For every default rule which has no token type, make sure each one-character
// pattern that isn't handled (explicitly matched or passed on) by the base
// state is matched by the target state. (This is a little conservative. It
// assumes there isn't a case where the base state covers xA, and the target
// state covers xB, where A and B cover everything,)
void checkDefaultProgress(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        int n = countRules(s->rules);
        rule *last = s->rules[n-1];
        if (! last->look) continue;
        if (last->tag != MORE) continue;
        if (last->patterns[0] != NULL) continue;
        char single[2] = "x";
        for (int ch = '!'; ch <= '~'; ch++) {
            single[0] = ch;
            checkAccept(lang, last->target, single);
        }
        checkAccept(lang, last->target, "\n");
        checkAccept(lang, last->target, " ");
    }
}

// ----- Sorting --------------------------------------------------------------

// Sort the states so that the start states come before the continuation
// states. Limit the number of start states to 32. Add indexes.
void sortStates(state *states[]) {
    int nstarters = 0;
    for (int i = 1; states[i] != NULL; i++) {
        state *s = states[i];
        int j = i - 1;
        if (! s->start) continue;
        nstarters++;
        while (j >= 0 && ! states[j]->start) {
            states[j + 1] = states[j];
            j--;
        }
        states[j + 1] = s;
    }
    if (nstarters > 32) crash("more than 32 start states", 0, "");
    for (int i = 0; states[i] != NULL; i++) states[i]->index = i;
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
    if (! p->look && q->look) return -1;
    if (p->look && ! q->look) return 1;
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

// ----- Building -------------------------------------------------------------

// Fill in the actions for a non-default rule.
void addRuleActions(language *lang, state *s, rule *r) {
    for (int i = 0; r->patterns[i] != NULL; i++) {
        int p = r->patterns[i]->index;
        if (s->actions[p].tag != SKIP) continue;
        s->actions[p].tag = r->tag;
        s->actions[p].target = r->target;
    }
}

// Fill in the actions for a state.
void addActions(language *lang, state *s) {
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        s->actions[i].tag = SKIP;
    }
    for (int i = 0; s->rules[i] != NULL; i++) {
        addRuleActions(lang, s, s->rules[i]);
    }
}

// =============================================================================
/*


// Fill a non-default rule into the table.
void fillRule(
    entry table[][LIST], char *tokens[], char *states[], char *patterns[]
) {
    int n = length(tokens);
    byte action = (byte) tokens[n-1][0];
    byte state = (byte) find(states, tokens[0]);
    byte target = (byte) find(states, tokens[n-2]);
    for (int i = 1; i < n-2; i++) {
        short p = (short) find(patterns, tokens[i]);
        byte oldAction = table[state][p].action;
        if (oldAction != SKIP) continue;
        table[state][p].action = action;
        table[state][p].target = target;
    }
}

// Fill a default rule into the table.
void fillDefault(
    entry table[][LIST], char *tokens[], char *states[], char *patterns[]
) {
    byte action = (byte) tokens[2][0];
    byte state = (byte) find(states, tokens[0]);
    byte target = (byte) find(states, tokens[1]);
    char s[3] = "x?";
    for (int ch = 0; ch < 128; ch++) {
        if (ch < ' ' && ch != '\n') continue;
        if (ch == 127) continue;
        s[0] = ch;
        int p = find(patterns, s);
        table[state][p].action = action;
        table[state][p].target = target;
    }
}
*/
/*
// Check for any missing defaults, and complain.
void checkMissing(
    entry table[][LIST], char *states[], char *patterns[]
) {
    for (int s = 0; s < length(states); s++) {
        int n = length(patterns) - 1;
        if (table[s][n].action != SKIP) continue;
        crash("Default rule needed for state", 0, states[s]);
    }
}
*/
/*
// Enter rules into the table.
void fillTable(
    entry table[][LIST], int n, char *tokens[n][NAME],
    char *states[], char *patterns[]
) {
    for (int s=0; s<NAME; s++) for (int p=0; p<LIST; p++) {
        table[s][p].action = SKIP;
    }
    for (int i = 0; i < n; i++) {
        int t = length(tokens[i]);
        if (t == 3) fillDefault(table, tokens[i], states, patterns);
        else fillRule(table, tokens[i], states, patterns);
    }
//    checkMissing(table, states, patterns);
}

scanner *buildScanner(char const *path) {
    scanner *sc = calloc(1, sizeof(scanner));
    readFile(path, sc->text);
    splitLines(sc->text, sc->lines);
    makeSinglesTriples();
    splitTokens(sc->lines, sc->tokens);
    int nlines = length(sc->lines);
    sc->nstarts = gatherStates(nlines, sc->tokens, sc->states);
    gatherPatterns(nlines, sc->tokens, sc->patterns);
    sort(sc->patterns);
    sc->nstates = length(sc->states);
    sc->npatterns = length(sc->patterns);
    fillTable(sc->table, nlines, sc->tokens, sc->states, sc->patterns);
    return sc;
}

// Write out a row of state names (for tracing) then a row per pattern with the
// pattern string on the end, and finally the defaults row.
void writeScanner(scanner *sc, char const *path) {
    FILE *fp = fopen(path, "w");
    char *stateLabels =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < sc->nstates; i++) {
        char *s = sc->states[i];
        if (i > 0) fprintf(fp, " ");
        fprintf(fp, "%s", s);
    }
    fprintf(fp, "\n");
    for (int p = 0; p < sc->npatterns; p++) {
        for (int s = 0; s < sc->nstates; s++) {
            char action = (char) sc->table[s][p].action;
            char target = stateLabels[sc->table[s][p].target];
            fprintf(fp, "%c%c ", action, target);
        }
        if (strlen(sc->patterns[p]) == 0) fprintf(fp, " default\n");
        else fprintf(fp, " %s\n", sc->patterns[p]);
    }
    fclose(fp);
}

// ----- Testing --------------------------------------------------------------

// Check two NULL terminated arrays of strings are equal.
bool eq(char *as[], char *bs[]) {
    if (length(as) != length(bs)) return false;
    for (int i = 0; i < length(as); i++) {
        if (strcmp(as[i],bs[i]) != 0) return false;
    }
    return bs[length(as)] == NULL;
}

void testSplitLines() {
    char s[] = "abc\ndef\n\nghi\n";
    char *lines[10] = {NULL};
    char *expect[] = {"abc", "def", "ghi", NULL};
    splitLines(s, lines);
    assert(eq(lines, expect));
}

void testSplitTokens() {
    char s1[20] = "s a b c t", s2[20] = " s  \\s \\b  t  ", s3[20] = "s a..c t";
    char s4[20] = "s a t X";
    char *lines[5] = {s1, s2, s3, s4, NULL};
    char *expect[4][8] = {
        {"s", "a", "b", "c", "t", "-", NULL},
        {"s", "\\s", "\\b", "t", "-", NULL},
        {"s", "a", "b", "c", "t", "-", NULL},
        {"s", "a", "t", "X", NULL},
    };
    char *tokens[4][NAME] = {{NULL},{NULL},{NULL},{NULL}};
    makeSinglesTriples();
    splitTokens(lines, tokens);
    assert(eq(tokens[0], expect[0]));
    assert(eq(tokens[1], expect[1]));
    assert(eq(tokens[2], expect[2]));
    assert(eq(tokens[3], expect[3]));
}

void testGatherStates() {
    char *ts[2][NAME] = {{"s0","?","s1","X",NULL}, {"s0","s2","X",NULL}};
    char *states[10] = {NULL};
    char *expect[] = {"s0", "s1", "s2", NULL};
    gatherStates(2, ts, states);
    assert(eq(states, expect));
}

void testGatherPatterns() {
    char *ts[2][NAME] = {{"s","x","s","X",NULL}, {"s","y","s","X",NULL}};
    char *patterns[200] = {NULL};
    gatherPatterns(2, ts, patterns);
    assert(strcmp(patterns[96],"x") == 0);
    assert(strcmp(patterns[97],"y") == 0);
}

void testSort() {
    assert(compare("!","<") == -1);
    assert(compare("<=","<") == -1);
    assert(compare("<","<=") == 1);
    assert(compare("<","<?") == -1);
    assert(compare("<?","<") == 1);
    assert(compare("<=","<?") == -1);
    assert(compare("<?","<=") == 1);

    char *patterns[] = { "<?", "", "<", "<=", "!", NULL, NULL };
    char *expect[] = {"!", "<=", "<", "<?", "", NULL };
    sort(patterns);
    printf("p=%s %s\n", patterns[2], patterns[3]);
    assert(eq(patterns, expect));
}
*/
int main(int n, char const *args[n]) {
    language *lang = readLanguage("c/rules.txt");
    int nr = countRules(lang->rules);
    int ns = countStates(lang->states);
    int np = countPatterns(lang->patterns);
    printf("rules %d states %d patterns %d\n", nr, ns, np);
    findStartStates(lang);
    rule *r = lang->rules[0];
    printf("start %d %d\n", r->row, countPatterns(r->patterns));
    checkAccessible(lang);
    addDefaults(lang);
    checkTransfers(lang);
    checkPatternProgress(lang);
    checkDefaultProgress(lang);
    sortStates(lang->states);
    for (int i = 0; lang->states[i] != NULL; i++) {
        printf("%s %d\n", lang->states[i]->name, lang->states[i]->start);
    }
    printf("\n");
    sortPatterns(lang->patterns);
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        printf("%s %d\n", lang->patterns[i]->name, lang->patterns[i]->look);
    }
    freeLanguage(lang);
    /*
    testSplitLines();
    testSplitTokens();
    testGatherStates();
    testGatherPatterns();
    testSort();
    */
    /*
    if (n != 2) crash("Use: ./compile language", 0, "");
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    scanner *sc = buildScanner(path);
    sprintf(path, "%s/table.txt", args[1]);
    writeScanner(sc, path);
    free(sc);
    */
    return 0;
}
