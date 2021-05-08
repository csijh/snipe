// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdbool.h>

// Patterns are distinct strings stored in a list.

// Find or add a pattern, returning the version stored in the list.
char *findOrAddPattern(strings *patterns, char *p);

// Find or add a one-character pattern string, returning its index. (Memory is
// not allocated for the string, it is stored in static memory.)
char *findOrAddPattern1(strings *patterns, char pc);

// Sort patterns into ASCII order, except prefixes come after longer strings.
void sortPatterns(strings *patterns);

// Find the index of a pattern string, or -1 if not present.
int findPattern(strings *patterns, char *p);
