// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdbool.h>

// The rules are kept in a list.
struct rules;
typedef struct rules rules;

// Create or free the list of rules.
rules *newRules(char MORE);
void freeRules(rules *rs);

// Read a rule, if any, from the tokens on a given line.
void readRule(rules *rs, int row, strings *tokens);
