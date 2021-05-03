// Snipe language compiler. Free and open source. See licence.txt.

// Provide a pair of stacks of integers in a gap buffer. Functions with suffix L
// and R refer to the left and right stack.
struct stacks;
typedef struct stacks stacks;

// Give error message, with printf-style parameters, and exit.
void crash(char const *message, ...);

// Create a new pair of stacks.
stacks *newStacks();

// Push an integer onto one of the stacks.
void pushL(stacks *ss, int i);
void pushR(stacks *ss, int i);

// Get the most recent integer in one of the stacks, or -1.
int topL(stacks *ss);
int topR(stacks *ss);

// Take off the most recent integer from one of the stacks.
int popL(stacks *ss);
int popR(stacks *ss);
