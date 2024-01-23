// The Snipe editor is free and open source. See licence.txt.

// Given a language as a state machine and an initial state, scan the input from
// a given position up to its length (usually one line). Fill in token types in
// the out array. Carry out bracket matching using the given stack, which must
// have sufficient capacity ensured in advance. If the array of state names is
// not NULL, use it to print a trace of the execution. Return the final state.
int scan(byte *lang, int s0, char *in, int at, byte *out, int *stk, char **ns);
