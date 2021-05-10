// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdbool.h>

// The states are kept in a list, mostly accessed by name.
struct states;
typedef struct states states;

// Create or free the list of states.
states *newStates();
void freeStates(states *ss);

// Add a state with a given name, if not already defined.
void addState(states *ss, char *name);

// Set state to be a starting state or continuing state according to the flag.
void setType(states *ss, char *name, bool starting);

// Check whether a state is a starting state or not.
// bool isStarting(states *ss, char *name);

// Convert a rule into actions in a state. Pass the rule info in as a line no.,
// base state, patterns, target state, and tag with possible lookahead flag.
// Check that a default continuing rule has base and target states which are
// both starting or both continuing.
void convert(states *ss, int row, char *b, strings *ps, char *t, char tag);

// Sort the states with starting states first and allocate index numbers. Limit
// the number of starting states to 32, and the total number of states to 128.
void sortStates(states *ss);

// Prepare to fill in actions, given the sorted patterns and the SKIP tag.
void setupActions(states *ss, strings *patterns);

// Fill in a state's action for a pattern, with a tag and target state.
void fillAction(states *ss, char *name, int p, char tag, char *target);

// Check that all states have rules/actions associated with them.
//void checkDefined(states *ss);

// Check that each state covers all input characters.
void checkComplete(states *ss);

// Check the actions to ensure that progress is always made.
void checkProgress(states *ss);

// Write out a binary file containing the names of the states as null-terminated
// strings, then a null, then the pattern strings, then a null, then the array
// of actions for each state.
void writeTable(states *ss, char const *path);
