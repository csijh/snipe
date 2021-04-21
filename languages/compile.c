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
// !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~

// A tag is an upper case name or one of the above symbols, which include MORE
// to indicate continuing the current token and BAD to indicate an error token.
// A tag can also be SKIP to mean ignore a table entry as not relevant to the
// current state (also used later in the scanner to tag bytes which continue a
// character), or GAP to tag a space as a gap between tokens or NL to tag a
// newline as a token.
enum tag { MORE = '-', BAD = '?', SKIP = '~', GAP = '_', NL = '.' };

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

// A state has a name, a flag to indicate whether it has been added as a
// default, a flag to say if it is a starting state or a continuing state, and a
// proof (i.e. the line number of the rule where the flag was set, or 0 if the
// flag hasn't been set yet). The rules which define the state are listed.
// Later, an index is assigned, actions are added, and the visiting and visited
// flags are used.
struct state {
    char name[SMALL];
    bool added, starting;
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
    s->added = false;
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
    strcpy(tag->name, name);
    addPattern(lang->tags, tag);
    if ('A' <= name[0] && name[0] <= 'Z') return tag;
    if (strlen(name) > 1) crash("tag is more than one symbol", row, name);
    if (row == 0) return tag;
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
    lang->newline = findTag1(lang, 0, NL);
    return lang;
}

void freeLanguage(language *lang) {
    for (int i = 0; lang->patterns[i] != NULL; i++) free(lang->patterns[i]);
    for (int i = 0; lang->states[i] != NULL; i++) free(lang->states[i]);
    for (int i = 0; lang->rules[i] != NULL; i++) free(lang->rules[i]);
    for (int i = 0; lang->tags[i] != NULL; i++) free(lang->tags[i]);
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

// Combine the above functions to check the consistency of the language.
void checkLanguage(language *lang) {
    checkDefined(lang);
    findStartStates(lang);
    checkLookaheads(lang);
    checkTransfers(lang);
    checkAccessible(lang);
}

// ----- Defaults --------------------------------------------------------------

// Add implicit defaults for a starting state. If there is an explicit default
// rule (an unconditional jump to another starting state) or a rule which
// mentions the range !..~ then all characters are covered. If not, add a rule
// which accepts any single character as an error. Add space and newline rules.
void addStartingDefaults(language *lang, state *s) {
    bool ok = false;
    for (int j = 0; s->rules[j] != NULL; j++) {
        rule *r = s->rules[j];
        if (r->type == DEFAULT) ok = true;
        if (r->patterns[0] == NULL) continue;
        if (strcmp(r->patterns[0]->name, "!..~") == 0) ok = true;
    }
    if (! ok) {
        char *bad[] = { s->name, "!..~", s->name, lang->bad->name, NULL};
        readRule(lang, 0, bad);
    }
    char *sp[] = { s->name, " ", s->name, lang->gap->name, NULL};
    readRule(lang, 0, sp);
    char *nl[] = { s->name, "\n", s->name, lang->newline->name, NULL};
    readRule(lang, 0, nl);
}

// Add implicit defaults for a continuing rule. If necessary add "s start ?". If
// there are no explicit lookahead rules, add "D s \s d" (where D and d are from
// the default rule). Otherwise, add "_ s \s s" to indicate looking ahead past
// spaces. Add "D s \n d".
void addContinuingDefaults(language *lang, state *s) {
    int n = countRules(s->rules);
    rule *d = s->rules[n-1];
    if (d->type != DEFAULT) {
        state *s0 = lang->states[0];
        char *df[] = { s->name, s0->name, lang->bad->name, NULL };
        readRule(lang, 0, df);
        d = s->rules[n];
    }
    bool hasLookaheads = false;
    for (int j = 0; s->rules[j] != NULL; j++) {
        rule *r = s->rules[j];
        if (r->type == LOOKAHEAD) hasLookaheads = true;
    }
    if (! hasLookaheads) {
        char *df[] = { d->tag->name, s->name, " ", d->target->name, NULL };
        readRule(lang, 0, df);
    }
    else {
        char *df[] = { lang->gap->name, s->name, " ", s->name, NULL };
        readRule(lang, 0, df);
    }
    char *nl[] = { d->tag->name, s->name, "\n", d->target->name, NULL };
    readRule(lang, 0, nl);
}

// Combine the above functions to add the default rules to the language.
void addDefaults(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        if (s->added) continue;
        if (s->starting) addStartingDefaults(lang, s);
        else addContinuingDefaults(lang, s);
    }
}

// ----- Interim printing ------------------------------------------------------

// Print out a state in original style, to conform additions.
void printState(language *lang, state *s) {
    for (int i = 0; s->rules[i] != 0; i++) {
        rule *r = s->rules[i];
        if (r->row == 0) printf("%-4s", "+");
        else printf("%-4d", r->row);
        if (r->type == LOOKAHEAD) printf("%s ", r->tag->name);
        printf("%s", s->name);
        for (int j = 0; r->patterns[j] != NULL; j++) {
            pattern *p = r->patterns[j];
            if (p->name[0] == '\n') printf(" \\n");
            else if (p->name[0] == ' ') printf(" \\s");
            else if (p->name[0] == '\\') printf(" \\%s", p->name);
            else printf(" %s", p->name);
        }
        printf(" %s", r->target->name);
        if (r->type != LOOKAHEAD) printf(" %s", r->tag->name);
        printf("\n");
    }
}

// Print out all the states, to confirm additions before generating actions.
void printLanguage(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        printState(lang, lang->states[i]);
        printf("\n");
    }
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

// Check whether a pattern string is a range of characters.
bool isRange(char *s) {
    return strlen(s) == 4 && s[1] == '.' && s[2] == '.';
}

// Compare two patterns in ASCII order, except prefer longer strings and
// put ranges last.
int compare(pattern *p, pattern *q) {
    char *s = p->name, *t = q->name;
    if (isRange(s) && ! isRange(t)) return 1;
    if (isRange(t) && ! isRange(s)) return -1;
    for (int i = 0; ; i++) {
        if (s[i] == '\0' && t[i] == '\0') break;
        if (s[i] == '\0') return 1;
        if (t[i] == '\0') return -1;
        if (s[i] < t[i]) return -1;
        if (s[i] > t[i]) return 1;
    }
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

// Remove ranges from the patterns array, and add them to the tags array so they
// get freed at the end.
void removeRanges(language *lang) {
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (! isRange(p->name)) continue;
        lang->patterns[i] = NULL;
        addPattern(lang->tags, p);
    }
}

// Sort the states and patterns.
void sort(language *lang) {
    sortStates(lang->states);
    sortPatterns(lang->patterns);
    removeRanges(lang);
}

// ----- Actions ---------------------------------------------------------------

// Initialize all actions to SKIP.
void fillSkipActions(language *lang) {
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        for (int j = 0; lang->patterns[j] != NULL; j++) {
            s->actions[j].tag = SKIP;
            s->actions[j].target = 0;
        }
    }
}

// Fill in an action for a particular rule and pattern, if not already filled
// in. For a lookahead or default rule, set the top bit of the tag byte.
void fillAction(language *lang, rule *r, pattern *p) {
    state *s = r->base;
    int n = p->index;
    if (s->actions[n].tag != SKIP) return;
    s->actions[n].tag = r->tag->name[0];
    if (r->type != MATCHING) s->actions[n].tag |= 0x80;
    s->actions[n].target = r->target->index;
}

// Fill in a range of one-character actions for a rule.
void fillRangeActions(language *lang, rule *r, char from, char to) {
    for (char ch = from; ch <= to; ch++) {
        if (ch < '\n' || ('\n' < ch && ch < ' ') || ch > '~') continue;
        pattern *p = findPattern1(lang, ch);
        fillAction(lang, r, p);
    }
}

// Fill in the actions for a rule, for patterns not already handled.
void fillRuleActions(language *lang, rule *r) {
    if (r->type == DEFAULT) fillRangeActions(lang, r, '\n', '~');
    else for (int i = 0; r->patterns[i] != NULL; i++) {
        pattern *p = r->patterns[i];
        if (isRange(p->name)) fillRangeActions(lang, r, p->name[0], p->name[3]);
        else fillAction(lang, r, p);
    }
}

// Fill in all actions.
void fillActions(language *lang) {
    fillSkipActions(lang);
    for (int i = 0; lang->rules[i] != NULL; i++) {
        rule *r = lang->rules[i];
        fillRuleActions(lang, r);
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

// Visit a state during a depth first search, looking for a loop with no
// progress when the given character is next in the input. A lookahead action
// for a pattern starting with the character indicates a progress-free jump to
// another state. Any (non-skip) action for a pattern which is that single
// character indicates that the character has been dealt with. Return true for
// success, false for a loop.
bool visit(language *lang, state *s, char ch) {
    if (s->visited) return true;
    if (s->visiting) return false;
    s->visiting = true;
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        if (p->name[0] < ch) continue;
        if (p->name[0] > ch) break;
        int tag = s->actions[i].tag;
        if (tag == SKIP) continue;
        bool lookahead = (tag & 0x80) != 0;
        if (lookahead) {
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

// Read a language, analyse, add defaults, sort, fill actions, check progress.
// Optionally print before sorting.
language *buildLanguage(char *text, bool print) {
    language *lang = readLanguage(text);
    checkLanguage(lang);
    addDefaults(lang);
    if (print) printLanguage(lang);
    sort(lang);
    fillActions(lang);
    checkProgress(lang);
    return lang;
}

// ---------- Output -----------------------------------------------------------

// Write out a binary file containing the names of the states as null-terminated
// strings, then a null, then the pattern strings, then a null, then the array
// of actions for each state.
void writeTable(language *lang, char const *path) {
    FILE *fp = fopen(path, "wb");
    for (int i = 0; lang->states[i] != NULL; i++) {
        state *s = lang->states[i];
        fprintf(fp, "%s%c", s->name, '\0');
    }
    fprintf(fp, "%c", '\0');
    for (int i = 0; lang->patterns[i] != NULL; i++) {
        pattern *p = lang->patterns[i];
        fprintf(fp, "%s%c", p->name, '\0');
    }
    fprintf(fp, "%c", '\0');
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
    unsigned char tag;
    if (islower(tokens[0][0])) {
        s = findState(lang, tokens[0]);
        if (strcmp(tokens[1],"\\n") == 0) tokens[1] = "\n";
        if (strcmp(tokens[1],"\\s") == 0) tokens[1] = " ";
        p = findPattern(lang, tokens[1]);
        t = findState(lang, tokens[2]);
        tag = tokens[3][0];
    }
    else {
        s = findState(lang, tokens[1]);
        if (strcmp(tokens[2],"\\n") == 0) tokens[2] = "\n";
        if (strcmp(tokens[2],"\\s") == 0) tokens[2] = " ";
        p = findPattern(lang, tokens[2]);
        t = findState(lang, tokens[3]);
        tag = tokens[0][0] | 0x80;
    }
    unsigned char actionTag = s->actions[p->index].tag;
    int actionTarget = s->actions[p->index].target;
    if (actionTag == tag && actionTarget == t->index) return;
    fprintf(stderr, "Test failed: %s: %s (%d %c %s)\n",
        name, test, (actionTag >> 7), (actionTag & 0x7F),
        lang->states[actionTarget]->name);
    exit(1);
}

// Run the tests in an example.
void runExample(char *name, char *eg[], bool print) {
    char text[1000];
    strcpy(text, eg[0]);
    language *lang = buildLanguage(text, print);
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
    runExample("eg1", eg1, false);
    runExample("eg2", eg2, false);
    runExample("eg3", eg3, false);
    runExample("eg4", eg4, false);
    runExample("eg5", eg5, false);
    runExample("eg6", eg6, false);
    runExample("eg7", eg7, false);
    runExample("eg8", eg8, false);
    runExample("eg9", eg9, false);
    runExample("eg10", eg10, false);
    runExample("eg11", eg11, false);
    runExample("eg12", eg12, false);
//    runExample("eg13", eg13, false);
//    runExample("eg14", eg14, false);
//    runExample("eg15", eg15, false);
//    runExample("eg16", eg16, false);
}

int main(int n, char const *args[n]) {
    if (n != 2) crash("Use: ./compile language", 0, "");
    runTests();
    char path[100];
    sprintf(path, "%s/rules.txt", args[1]);
    char *text = readFile(path);
    language *lang = buildLanguage(text, true);
    sprintf(path, "%s/table.bin", args[1]);
    writeTable(lang, path);
    freeLanguage(lang);
    free(text);
    return 0;
}
