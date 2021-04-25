// Snipe language compiler. Free and open source. See licence.txt.

// Provide list handling functions, for lists of pointers, implemented as
// flexible arrays. Make forward references to the structure types.
struct rule;
typedef struct rule rule;

struct state;
typedef struct state state;

struct pattern;
typedef struct pattern pattern;

// Give error message, with line number or 0, and extra info or "", and exit.
void crash(char const *message, int row, char const *s);

// Create a new list of pointers.
char **newStrings();
rule **newRules();
state **newStates();
pattern **newPatterns();

// Free up a list.
void freeStrings(char *list[]);
void freeRules(rule *list[]);
void freeStates(state *list[]);
void freePatterns(pattern *list[]);

/*
// Ensure space for n more pointers. Return the possibly reallocated list.
char **checkStrings(char *list[], int n);
rule **checkRules(rule *list[], int n);
state **checkStates(state *list[], int n);
pattern **checkPatterns(pattern *list[], int n);
*/

// Add an item to a list, returning the possibly reallocated list.
char **addString(char *list[], char *s);
rule **addRule(rule *list[], rule *r);
state **addState(state *list[], state *r);
pattern **addPattern(pattern *list[], pattern *p);

// Find the length of a list.
int countStrings(char *list[]);
int countRules(rule *list[]);
int countStates(state *list[]);
int countPatterns(pattern *list[]);
