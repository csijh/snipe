// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdbool.h>

// A rule object holds the pieces extracted from a line of text: a row (line
// number), a base state, patterns, a target state, an optional token type (""
// if missing), and an optional '+' sign (lookahead flag). The patterns have
// numerical escapes for control characters or spaces translated, ranges x..y
// expanded, and implicit \0..\127 added. The type synonym 'rule' provides
// read-only access.
struct rule {
    int row;
    char *base;
    strings *patterns;
    char *target;
    char *type;
    bool lookahead;
};
typedef struct rule const rule;

// The rules are kept in a list.
struct rules;
typedef struct rules rules;

// Read rules from the given multi-line text. Discard comment lines (which start
// with a symbol), split each remaining line into tokens separated by spaces,
// convert into a rule, and add the rule to the list. Deal with escape
// sequences, expand ranges, and insert \1..\127 if there are no patterns.
rules *newRules(char *text);

// Free the list of rules.
void freeRules(rules *rs);

// Return the length of the list.
int countRules(rules *rs);

// Get the i'th rule.
rule *getRule(rules *rs, int i);

// Get the sorted list of patterns gathered from the rules. A pattern may
// contain any ASCII character except '\0'.
strings *getPatterns(rules *rs);

// Get the sorted list of token types (including "") gathered from the rules.
strings *getTypes(rules *rs);
