// Snipe editor. Free and open source, see licence.txt.

// Scanning gives a type to each token of text. The bytes of a token after the
// first are marked with None. Bracket types come in matching opener and closer
// pairs, with a B or E suffix. The Comment and Bad flags are used to flag a
// token, reversibly, as commented or mismatched.

enum type {
    None, Gap, Newline, Alternative, Declaration, Function, Identifier, Join,
    Keyword, Long, Mark, NoteD, Note, Operator, Property, QuoteD, Quote, Tag,
    Unary, Value, Wrong,

    LongB, CommentB, CommentNB, TagB, RoundB, Round2B, SquareB, Square2B,
    GroupB, Group2B, BlockB, Block2B,

    LongE, CommentE, CommentNE, TagE, RoundE, Round2E, SquareE, Square2E,
    GroupE, Group2E, BlockE, Block2E,

    FirstB = LongB, LastB = Block2B, FirstE = LongE, LastE = Block2E,
    Comment = 64, Bad = 128,
};

// The scanner also handles bracket matching.
bool isOpener(int type);
bool isCloser(int type);
bool match(int opener, int closer);

// Given a language as a state machine and an initial state, scan the input from
// a given position up to its length (usually one line). Fill in token types in
// the out array. Carry out bracket matching using the given stack, which must
// have sufficient capacity ensured in advance. If the array of state names is
// not NULL, use it to print a trace of the execution. Return the final state.
int scan(byte *lang, int s0, char *in, int at, byte *out, int *stk, char **ns);
