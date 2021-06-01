// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include "rules.h"
#include <stdbool.h>

// The states are kept in a list, and accessed by name.
struct states;
typedef struct states states;

// An action contains an op and a target state index, each stored in one byte.
// There is one action in each state for each pattern. The op is SKIP = 0xFF to
// mean the pattern is not relevant in the state, otherwise it is an index into
// the list of token type names, plus a flag (top bit) to indicate a lookahead action.
struct action { unsigned char op, target; };
typedef struct action action;

// Create the list of states from the list of rules.
states *newStates(rules *rs);

// Free the list of states.
void freeStates(states *ss);

// Prepare the states for output, and carry out checks, returning an error
// message or NULL. Check that there are no more than 128 states. Check whether
// each state is a starting state or continuing state, and check consistency.
// The base state of the first rule is a starting state. Any rule which
// terminates a token has a starting state as its target. A non-lookahead rule
// which continues a token has a continuing state as its target. A lookahead
// rule which terminates a token has a continuing state as its base. A lookahead
// rule which continues a token has base and target states which are both
// starting or both continuing. Check that each state is complete, covering all
// input characters. Check that there are no infinite loops, so progress is
// always made.
char *checkAndFillActions(states *ss);

// Extract one action, for a state and pattern, for testing.
action getAction(states *ss, char *s, char *p);

// Find the index of a state, for testing.
int getIndex(states *ss, char *s);

// Write out a binary file containing the names of the states as null-terminated
// strings, then a null, then the pattern strings, then a null, then the array
// of actions for each state.
void writeTable(states *ss, char const *path);
