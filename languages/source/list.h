// Snipe language compiler. Free and open source. See licence.txt.

// Support lists of pointers as flexible NULL-terminated arrays. A list can be
// added to with "add(&list,x)" to allow for reallocation, iterated with "for
// (i=0; list[i]!=NULL; i++)" cleared with "list[0] = NULL",  and deallocated
// with free(list).

// Make forward references to the structure types.
struct rule;
typedef struct rule rule;

struct state;
typedef struct state state;

struct pattern;
typedef struct pattern pattern;

struct tag;
typedef struct tag const tag;

struct quad;
typedef struct quad const quad;

// Give error message, with line number or 0, and extra info or "", and exit.
void crash(char const *message, ...);

// Create a new list of pointers.
char **newStrings();
rule **newRules();
state **newStates();
pattern **newPatterns();
tag **newTags();

// Given a pointer to a list, add an item to the list, possibly reallocated it.
void addString(char ***listp, char *s);
void addRule(rule ***listp, rule *r);
void addState(state ***listp, state *r);
void addPattern(pattern ***listp, pattern *p);
void addTag(tag ***listp, tag *t);
void addQuad(quad ***listp, quad *t);

// Find the length of a list.
int countStrings(char *list[]);
int countRules(rule *list[]);
int countStates(state *list[]);
int countPatterns(pattern *list[]);
int countTags(tag *list[]);
