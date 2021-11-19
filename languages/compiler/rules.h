// Snipe language compiler. Free and open source. See licence.txt.
#include "lists.h"
#include <stdbool.h>

// A rule object holds the pieces extracted from a line of text: a row (line
// number), a base state, patterns, a target state, an optional token type (""
// if missing), and an optional '+' sign (lookahead flag). The patterns have
// numerical escapes for control characters or spaces translated, and ranges
// x..y expanded.
struct rule;
typedef struct rule rule;

// Extract the pieces from a rule.
int row(rule *r);
string *base(rule *r);
list *patterns(rule *r);
string *target(rule *r);
string *type(rule *r);
bool lookahead(rule *r);

// The rules are kept in a list.
struct rules;
typedef struct rules rules;

// Create a list of rules from a list of lines. Discard comment lines (which
// start with a symbol), split each remaining line at the spaces, convert into a
// rule, and add the rule to the list. Deal with escape sequences.
rules *newRules(list *lines);

// Free a list of rules.
void freeRules(rules *rs);

// Return the length of a list of rules.
int count(rules *rs);

// Get the i'th rule.
rule *getRule(rules *rs, int i);

/*
// Get the sorted list of patterns gathered from the rules. A pattern may
// contain any ASCII character except '\0'.
strings *getPatterns(rules *rs);

// Get the sorted list of token types (including "") gathered from the rules.
strings *getTypes(rules *rs);
*/
