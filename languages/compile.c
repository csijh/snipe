// TODO sort out lookahead past spaces (state')
// TODO override only if normal AND lookahead not set.
// TODO three-way rule type.
// TODO go back to a flag in the tag to say lookahead action.
// TODO wait until action generation to expand ranges?
// BUT how express as a pattern? Left with spare patterns?
// SORT -> put ranges last and then ignore.

// Snipe language compiler. Free and open source. See licence.txt.

// Type  ./compile x  to compile a description of language x in x/rules.txt into
// a scanner table in x/table.bin. The program interpret.c can be used to test
// the table. A rule has a base state, patterns, a target state, and an optional
// tag at the beginning or end. A pattern may be a range such as 0..9
// representing many single-character patterns. A tag at the beginning of a rule
// indicates a lookahead rule, and a lack of patterns indicates a default rule.

// Each state must consistently be either a starting state between tokens, or a
// continuing state within tokens. A default rule is treated as a lookahead for
// any single-character patterns not already covered, so that the state machine
// operation is uniformly driven by the next input character. The implicit
// default, if there is no explicit one, for a starting state is to accept and
// suitably tag a space or newline, or to accept any other single character as
// an error token, and stay in the same state. The implicit default for a
// continuing state is to terminate the current token as an error token and go
// to the initial state. There is a search for cycles which make no progress.

// The resulting table has an entry for each state and pattern, with a tag and a
// target. The tag is a token type to label and terminate the current token, or
// indicates continuing the token, skipping the table entry, or classifying a
// text byte as a continuing byte of a character or a continuing character
// of a token or a space or a newline. The tag has its top bit set to indicate
// lookahead behaviour rather than normal matching behaviour.

// The states are sorted with starting states first, and the number of starting
// states is limited to 32 so they can be cached by the scanner (in the tag
// bytes for spaces). The total number of states is limited to 128 so a state
// index can be held in a char. The patterns are sorted, with longer ones before
// shorter ones. The next character in the input is used to find the first
// pattern starting with that character. The patterns are searched linearly,
// skipping the ones where the table entry has SKIP, to find the first match.
// The defaults ensure that the search always succeeds.

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

// A tag is an upper case name or one of the above symbols, which include MORE
// to indicate continuing the current token and BAD to indicate an error token.
// A tag can also be SKIP to mean ignore a table entry as not relevant to the
// current state (also used later in the scanner to tag bytes which continue a
// character), or GAP to tag a space as a gap between tokens or NEWLINE to tag a
// newline as a token.
enum tag { MORE = '-', BAD = '?', SKIP = '~', GAP = '_', NEWLINE = '.'};

// A rule is a matching rule, a lookahead rule (tag on left), or a default rule
// (no patterns).
enum type { MATCHING, LOOKAHEAD, DEFAULT };

// --------- Structures --------------------------------------------------------

// An action contains a tag and a target state, each stored in one byte.
struct action {
    unsigned char tag, target;
};
typedef struct action action;

// A pattern is a string, with an index added after sorting. Also used for tags.
struct pattern {
    char name[SMALL];
    int index;
};
typedef struct pattern pattern;

// A state has a name, a flag to indicate whether it is a starting state or a
// continuing state, and a proof (i.e. the line number of the rule where the
// flag was set, or 0 if the flag hasn't been set yet). The rules which define
// the state are listed. Later, an index is assigned, actions are added, and the
// visiting and visited flags are used.
struct state {
    char name[SMALL];
    bool starting;
    int proof;
    struct rule *rules[BIG];
    int index;
    action actions[BIG];
    bool visiting, visited;
};
typedef struct state state;

// A rule has a row (line number), a base state, patterns, a target state, and a
// tag. The default for a missing tag is MORE. The type indicates if the rule is
// a lookahead (tag at start) or a default (no patterns). The ending flag says
// if the rule ends a token (the tag is a token type) or extends the token (tag
// MORE).
struct rule {
    int row;
    state *base, *target;
    pattern *patterns[BIG];
    pattern *tag;
    int type;
    bool ending;
};
typedef struct rule rule;

// A language description has lists of rules, states, patterns and tags, and a
// few named tags.
struct language {
    rule *rules[BIG];
    state *states[BIG];
    pattern *patterns[BIG];
    pattern *tags[BIG];
    pattern *more, *bad, *skip, *gap, *newline;
};
typedef struct language language;

// ----- Lists -----------------------------------------------------------------
// It would be nice to have generic functions, but in C that is too ugly.

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
// initialize, and to iterate through without needing the length.
int addPattern(pattern *list[], pattern *p) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n] = p;
    if (n+1 >= BIG) crash("Too many patterns", 0, "");
    list[n+1] = NULL;
    return n;
}

int addState(state *list[], state *s) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n] =s;
    if (n+1 >= BIG) crash("Too many states", 0, "");
    list[n+1] = NULL;
    return n;
}

int addRule(rule *list[], rule *r) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n] = r;
    if (n+1 >= BIG) crash("Too many rules", 0, "");
    list[n+1] = NULL;
    return n;
}

int addString(char *list[], char *s) {
    int n = 0;
    while (list[n] != NULL) n++;
    list[n] = s;
    if (n+1 >= BIG) crash("Too many strings", 0, "");
    list[n+1] = NULL;
    return n;
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

// Find a pattern, or create a new one. Also used for tags.
pattern *findPattern(language *lang, char *name) {
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (strcmp(name, p->name) == 0) return p;
    }
    pattern *p = malloc(sizeof(pattern));
    addPattern(lang->patterns, p);
    if (strlen(name) >= SMALL) crash("name too long", 0, name);
    strcpy(p->name, name);
    return p;
}

// Find a one-character pattern, or create a new one.
pattern *findPattern1(language *lang, char ch) {
    char name[2] = "x";
    name[0] = ch;
    return findPattern(lang, name);
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

// Find a tag, returning its index. Report an error if two alphabetic names have
// the same first letter, or if a symbol is not one of the allowed ones.
pattern *findTag(language *lang, int row, char *name) {
    for (int i = 0; lang->tags[i] != NULL; i++) {
        pattern *p = lang->tags[i];
        if (p->name[0] != name[0]) continue;
        if (strcmp(p->name, name) == 0) return p;
        char tags[SMALL*2];
        sprintf(tags, "%s %s", name, p->name);
        crash("tags disagree in first letter", row, tags);
    }
    pattern *tag = malloc(sizeof(pattern));
    addPattern(lang->tags, tag);
    if ('A' <= name[0] && name[0] <= 'Z') return tag;
    if (strlen(name) > 1) crash("tag is more than one symbol", row, name);
    if (strchr(symbols, name[0]) == NULL) crash("bad token type", row, name);
    return tag;
}

// Find a one-character tag.
pattern *findTag1(language *lang, int row, char ch) {
    char name[] = "x";
    name[0] = ch;
    return findTag(lang, row, name);
}

// Add a pattern to a rule.
void readPattern(language *lang, rule *r, char *token) {
    pattern *p = findPattern(lang, token);
    addPattern(r->patterns, p);
}

// Read a rule, if any, from the tokens on a given line.
void readRule(language *lang, int row, char *tokens[]) {
    if (tokens[0] == NULL) return;
    if (strlen(tokens[0]) > 1 && ! isalpha(tokens[0][0])) return;
    rule *r = malloc(sizeof(rule));
    addRule(lang->rules, r);
    r->row = row;
    r->type = MATCHING;
    r->tag = NULL;
    int first = 0, last = countStrings(tokens) - 1;
    if (first >= last) crash("rule too short", row, "");
    if (! islower(tokens[first][0])) {
        r->tag = findTag(lang, row, tokens[first]);
        r->type = LOOKAHEAD;
        first++;
    }
    if (! islower(tokens[last][0])) {
        if (r->tag != NULL) crash("rule has a tag first and last", row, "");
        r->tag = findTag(lang, row, tokens[last]);
        last--;
    }
    if (r->tag == NULL) r->tag = lang->more;
    r->ending = true;
    if (r->tag == lang->more) r->ending = false;
    if (last == first + 1) r->type = DEFAULT;
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

// Create a language structure and add one-character patterns, and special tags.
language *newLanguage() {
    language *lang = malloc(sizeof(language));
    lang->rules[0] = NULL;
    lang->states[0] = NULL;
    lang->patterns[0] = NULL;
    findPattern(lang, "\n");
    findPattern(lang, " ");
    for (int ch = '!'; ch <= '~'; ch++) findPattern1(lang, ch);
    lang->more = findTag1(lang, 0, MORE);
    lang->bad = findTag1(lang, 0, BAD);
    lang->skip = findTag1(lang, 0, SKIP);
    lang->gap = findTag1(lang, 0, GAP);
    lang->newline = findTag1(lang, 0, NEWLINE);
    return lang;
}

void freeLanguage(language *lang) {
    for (int i = 0; lang->patterns[i] != NULL; i++) free(lang->patterns[i]);
    for (int i = 0; lang->states[i] != NULL; i++) free(lang->states[i]);
    for (int i = 0; lang->rules[i] != NULL; i++) free(lang->rules[i]);
    free(lang);
}

/*
// Find a tag character from a rule's token type.
char findTag(int row, char *name) {
    if ('A' <= name[0] && name[0] <= 'Z') return name[0];
    if (strlen(name) != 1) crash("bad token type", row, name);
    if (strchr(symbols, name[0]) == NULL) crash("bad token type", row, name);
    return name[0];
}
*/

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

// Set state s to be a starting state or continuing state according to the flag.
// Give error message if already found out to be the opposite.
void setStart(state *s, bool starting, int row) {
    if (s->proof > 0 && s->starting != starting) {
        fprintf(stderr, "Error: %s is a ", s->name);
        if (s->starting) fprintf(stderr, "starting");
        else fprintf(stderr, "continuing");
        fprintf(stderr, " state because of line %d\n", s->proof);
        if (starting) fprintf(stderr, "but can occur between tokens");
        else fprintf(stderr, "but can occur within a token");
        fprintf(stderr, " because of line %d\n", row);
        exit(1);
    }
    s->starting = starting;
    s->proof = row;
}

// Find starting states and continuing states, and check consistency.
// Any ending rule has a starting state as its target.
// A normal extending rule has a continuing state as its target.
// An ending lookahead or default rule has a continuing state as its base.
void findStartStates(language *lang) {
    state *s0 = lang->rules[0]->base;
    s0->starting = true;
    s0->proof = lang->rules[0]->row;
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (r->ending) {
            setStart(r->target, true, r->row);
        }
        if (r->type == MATCHING && ! r->ending) {
            setStart(r->target, false, r->row);
        }
        if (r->type != MATCHING && r->ending) {
            setStart(r->base, false, r->row);
        }
    }
}

// Check that an explicit lookahead rule in a continuing state has a token
// type (because there may be a space which must terminate the current token.)
void checkLookaheads(language *lang) {
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (! r->base->starting && r->type == LOOKAHEAD && ! r->ending) {
            fprintf(stderr, "Error in rule on line %d\n", r->row);
            fprintf(stderr, "an explicit lookahead rule in a ");
            fprintf(stderr, "continuing state must end the current token\n");
            exit(1);
        }
    }
}

// Check that an extending lookahead or default rule (i.e. a jump rule)
// has base and target states which are both starting states or both continuing.
void checkTransfers(language *lang) {
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        if (r->type == MATCHING || r->ending) continue;
        if (r->base->starting == r->target->starting) continue;
        fprintf(stderr, "Error in rule on line %d\n", r->row);
        if (r->base->starting) {
            fprintf(stderr, "%s is a starting state ", r->base->name);
            fprintf(stderr, "(line %d)\n", r->base->proof);
            fprintf(stderr, "but %s is a continuing state ", r->target->name);
            fprintf(stderr, "(line %d)\n", r->target->proof);
        }
        else {
            fprintf(stderr, "%s is a continuing state ", r->base->name);
            fprintf(stderr, "(line %d)\n", r->base->proof);
            fprintf(stderr, "but %s is a starting state ", r->target->name);
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
            if (r->type == DEFAULT) hasDefault = true;
            else if (hasDefault) crash("inaccessible rule", r->row, "");
        }
    }
}

// ----- Defaults --------------------------------------------------------------

// Add implicit defaults for a starting state. All starting states have space
// and newline rules added. If there is an explicit default rule (an
// unconditional jump to another starting state) or a rule which mentions the
// range !..~ then all other characters are covered. If not, add a rule which
// accepts any single character as an error token.
void addStartingDefaults(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (! s->starting) continue;
        char *sp[] = { s->name, " ", s->name, lang->gap->name, NULL};
        readRule(lang, 0, sp);
        char *nl[] = { s->name, "\n", s->name, lang->newline->name, NULL};
        readRule(lang, 0, nl);
        bool ok = false;
        for (int j = 0; s->rules[j] != NULL; j++) {
            rule *r = s->rules[j];
            if (r->type == DEFAULT) ok = true;
            if (strcmp(r->patterns[0]->name, "!..~") == 0) ok = true;
        }
        if (ok) continue;
        char *bad[] = { s->name, "!..~", s->name, lang->bad->name, NULL};
        readRule(lang, 0, bad);
    }
}

// Make a new related state with a name that isn't already being used.
state *freshState(language *lang, state *s) {
    char name[SMALL+20];
    int n = countStates(lang->states);
    for (int i = 1; ; i++) {
        sprintf(name, "%s%d", s->name, i);
        state *s = findState(lang, name);
        if (countStates(lang->states) > n) return s;
    }
}

// Copy a rule with base state s to form a rule with base state s1 and add it.
void copyRule(language *lang, rule *r, state *s, state *s1) {
    rule *r1 = malloc(sizeof(rule));
    *r1 = *r;
    r1->row = 0;
    r1->base = s1;
    addRule(lang->rules, r1);
    addRule(s1, r1);
}

// =============================================================================

// Add a new state s1 to deal with any lookahead rules in state s. The tag and
// target are from the default rule for s. State s1 has copies of the lookahead
// rules from state s, plus "s1 \n s1 .", "s1 \s s1 _" and "D s1 d".
state *addLookaheadState(language *lang, state *s, pattern *tag, state *t) {
    state *s1 = freshState(language *lang, state *s);
    for (int i = 0; s->rules[i] != NULL; i++) {
        rule *r = s->rules[i];
        if (r->type != LOOKAHEAD) continue;
        copyRule(lang, r, s, s1);
    }
    char *nl[] = { s1->name, "\n", s1->name, lang->newline->name, NULL};
    readRule(lang, 0, nl);
    char *sp[] = { s1->name, " ", s1->name, lang->gap->name, NULL};
    readRule(lang, 0, sp);
    char *df[] = { tag->name, s1->name, t->->name, NULL};
    return s1;
}

// Add implicit defaults for a continuing rule. If there is no explicit default,
// add "? s start". Add "D s \n d" (where D and d are from the default rule). If
// there are no explicit lookahead rules, add "D s \s d". Otherwise, add a new
// state s1, a rule "D s \s s1".
void addContinuingDefaults(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (s->starting) continue;
        int n = countRules(s->rules);
        rule *d = s->rules[n-1];
        if (d->type != DEFAULT) {
            state *s0 = lang->states[0];
            char *df[] = { lang->bad->name, s->name, s0->name, NULL };
            readRule(lang, 0, df);
            d = s->rules[n];
        }
        char *nl[] = { d->tag->name, s->name, "\n", d->target->name, NULL };
        readRule(lang, 0, nl);
        bool hasLookaheads = false;
        for (int j = 0; s->rules[j] != NULL; j++) {
            rule *r = s->rules[j];
            if (r->type == LOOKAHEAD) hasLookaheads = true;
        }
        if (! hasLookaheads) {
            char *df = { d->tag->name, s->name, " ", d->target->name, NULL };
            readRule(lang, df);
        }
        else {
            state *s1 = addLookaheadState(lang, s);
            char *df = {};
        }
    }
}
/*
// =============================================================================
// Add defaults for starting states:
// Add "start \n start ." and "start \s start _".
// If no explicit default, add "start !..~ start"
// (An explicit default is a jump to another starting state eg "start start2").
//
// Add defaults for a continuing state:
// If no explicit default, add "? id start", establishing a default tag.
// If state has an explicit (non-default) lookahead rule, add state id' with
// rules "X id \s id'", "id' \s id' GAP", "F id' ( start" [i.e. lookahead rules
// from id] and "X id' start" where
// Check whether a pattern string is a range of characters.
bool isRange(char *s) {
    return strlen(s) == 4 && s[1] == '.' && s[2] == '.';
}


// ----- Sorting --------------------------------------------------------------

// Sort the states so that the starting states come before the continuing
// states. Limit the number of starting states to 32, and the total number to
// 128. Add indexes.
void sortStates(state *states[]) {
    for (int i = 1; states[i] != NULL; i++) {
        state *s = states[i];
        int j = i - 1;
        if (! s->starting) continue;
        while (j >= 0 && ! states[j]->starting) {
            states[j + 1] = states[j];
            j--;
        }
        states[j + 1] = s;
    }
    for (int i = 0; states[i] != NULL; i++) {
        state *s = states[i];
        if (s->starting && i>=32) crash("more than 32 starting states", 0, "");
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
    if (! p->looking && q->looking) return -1;
    if (p->looking && ! q->looking) return 1;
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

// Fill in the implicit defaults for a starting state, for all match characters
// not already handled.
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
// continuing state, for all lookahead characters not already handled.
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
        if (s->starting) fillMatchActions(lang, s);
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
        if (p->looking) {
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
        if (p->looking) fprintf(fp, "|");
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
        if (p->looking) n = n | 0x80;
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

// A lookahead rule can be a continuing. It is a jump to another state.
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

// Can have multiple starting states. Check that the default is to accept any
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

// CAUSES ERROR. The prop state is a starting state because
// of line 3, and continuing state because of lines 4/5.
char *eg14[] = {
    "start . dot\n"
    "dot 0..9 start NUM\n"
    "SIGN dot a..z A..Z prop\n"
    "prop a..z A..Z 0..9 prop\n"
    "prop start PROPERTY\n",
    //----------------------
    "dot 0 start N", NULL
};

// CAUSES ERROR. An explicit lookahead rule in a continuing state must end the
// current token.
char *eg15[] = {
    "start a id\n"
    "- id ( id2\n"
    "id2 a id2\n"
    "id2 start ID\n",
    //----------------------
    "- id ( id2", NULL
};

// CAUSES ERROR. An explicit lookahead rule in a starting state must not end the
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
*/
int main(int n, char const *args[n]) {
    if (n != 2) crash("Use: ./compile language", 0, "");
/*
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
    */
    return 0;
}
