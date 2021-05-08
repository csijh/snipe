// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdbool.h>

// The states are kept in a list, mostly accessed by name.
struct states;
typedef struct states states;

// Create or free the list of states.
states *newStates();
void freeStates(states *ss);

// Set state to be a starting state or continuing state according to the flag.
// Give an error message if already found out to be the opposite.
void setType(states *ss, char *name, bool starting, int row);

// Check whether a state is a starting state or not.
bool isStarting(states *ss, char *name);

// Sort the states with starting states first and allocate index numbers. Limit
// the number of starting states to 32, and the total number of states to 128.
void sortStates(states *ss);

// Prepare to fill in actions, given the sorted patterns and the SKIP tag.
void setupActions(states *ss, strings *patterns, char SKIP);

// Fill in a state's action for a pattern, with a tag and target state.
void fillAction(states *ss, char *name, int p, char tag, char *target);

// Check that all states have rules/actions associated with them.
void checkDefined(states *ss);

// Check that each state covers all input characters.
void checkComplete(states *ss);

// Check the actions to ensure that progress is always made.
void checkProgress(states *ss, strings *patterns);

// Write out a binary file containing the names of the states as null-terminated
// strings, then a null, then the pattern strings, then a null, then the array
// of actions for each state.
void writeTable(states *ss, strings *patterns, char const *path);
