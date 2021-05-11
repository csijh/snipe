// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include "rules.h"
#include <stdbool.h>

// The states are kept in a list, and accessed by name.
struct states;
typedef struct states states;

// Create the list of states from the list of rules.
states *newStates(rules *rs);

// Free the list of states.
void freeStates(states *ss);

// Check whether each state is a starting state or continuing state, and check
// consistency. The base state of the first rule is a starting state. Any rule
// which terminates a token has a starting state as its target. A normal rule
// which continues a token has a continuing state as its target. A lookahead (or
// default) rule which terminates a token has a continuing state as its base.
void checkTypes(states *ss);

// Sort the states with starting states first and allocate index numbers. Limit
// the number of starting states to 32, and the total number of states to 128.
void sortStates(states *ss);

// Fill in the actions. Check that a default continuing rule has base and target
// states which are both starting or both continuing.
void fillActions(states *ss);

// Check that each state covers all input characters.
void checkComplete(states *ss);

// Check the actions to ensure that progress is always made.
void checkProgress(states *ss);

// Write out a binary file containing the names of the states as null-terminated
// strings, then a null, then the pattern strings, then a null, then the array
// of actions for each state.
void writeTable(states *ss, char const *path);
