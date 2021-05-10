// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdbool.h>

// A rule has a row (line number), a base state, patterns, a target state, a
// tag, and a lookahead flag. The rule type provides read-only access.
struct rule {
    int row;
    char *base, *target;
    strings *patterns;
    char *tag;
    bool lookahead;
};
typedef struct rule const rule;

// The rules are kept in a list.
struct rules;
typedef struct rules rules;

// Read the rules from the given file. Discard comment lines, split each
// remaining line into tokens, convert into a rule, and add the rule to the
// list. Deal with escaped characters, expand ranges, fill in a missing tag as
// the default "-", and implement a default rule (s t X) as a lookahead for each
// character (s \0..127 t ~X).
rules *readRules(char const *path);

// Free the list of rules.
void freeRules(rules *rs);

// Return the length of the list.
int countRules(rules *rs);

// Get the i'th rule.
rule *getRule(rules *rs, int i);

// Get the list of patterns gathered from the rules.
strings *getPatterns(rules *rs);

// Check that tag names are consistent.
void checkTags(rules *rs);

// Extract names of states, and check that all states are defined.
void stateNames(rules *rs, strings *names);

// Check whether a state is a starting state or continuing state, and check
// consistency. The base state of the first rule is a starting state. Any rule
// which terminates a token has a starting state as its target. A normal rule
// which continues a token has a continuing state as its target. A lookahead (or
// default) rule which terminates a token has a continuing state as its base.
bool isStarting(rules *rs, char *state);
