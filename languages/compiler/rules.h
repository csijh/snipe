// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdbool.h>

// A rule is represented by a line of text consisting of a base state, patterns,
// a target state, an optional token type, and an optional + sign as a lookahead
// flag. A pattern may contain numerical escapes for control characters or
// spaces, and may be a range x..y. In the rule structure, 'row' is the line
// number and NULL represents the absence of a token type. The type synonym
// 'rule' provides read-only access.
struct rule {
    int row;
    char *base;
    strings *patterns;
    char *target;
    bool lookahead;
    char *type;
};
typedef struct rule const rule;

// The rules are kept in a list.
struct rules;
typedef struct rules rules;

// Convert numerical escape sequences in a pattern string to characters, in
// place, replacing a null sequence \0 by the character 0x80. The row (line
// number) is used in generating an error message for a sequence outside the
// range \0..\127. Return the resulting length of the string.
int unescape(char *s, int row);

// Escape a pattern string in place, assuming sufficient memory, replacing
// control characters and spaces by decimal escape sequences. The string can
// contain any ASCII characters, with 0x80 in place of '\0'.
void escape(char *s);

// Read rules from the given multi-line text. Discard comment lines (which start
// with a symbol), split each remaining line into tokens separated by spaces,
// convert into a rule, and add the rule to the list. Deal with escape
// sequences, expand ranges, and insert \0..\127 if there are no patterns.
rules *readRules(char *text);

// Free the list of rules.
void freeRules(rules *rs);

// Return the length of the list.
int countRules(rules *rs);

// Get the i'th rule.
rule *getRule(rules *rs, int i);

// Get the sorted list of patterns gathered from the rules.
strings *getPatterns(rules *rs);

// Display a rule.
void printRule(rule *r);
