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

// Scan the text, using a language-specific table and a given start state, from
// at to length(text), usually one line, filling in the corresponding position
// in the types array. Update the stack of unmatched open brackets, which must
// have sufficient capacity ensured in advance. Return the final state.
int scan(byte *table, int state, int at, char *text, byte *types, int *stack);
