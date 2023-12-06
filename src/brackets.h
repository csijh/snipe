// Snipe editor. Free and open source, see licence.txt.

struct tokens;
typedef struct tokens Tokens;

// A type is used to mark a text byte, as a result of scanning. Delimiters have
// a D suffix, and bracket types come in matching pairs, with a B or E suffix.
// Special types and flags are:
//
//   None     marks token bytes after the first
//   Gap      marks a space or newline as a separator
//   Indent   marks an automatically managed indent
//   Cursor   a flag for display, indicating a cursor preceding the byte
//   Select   a background display flag, for a selection
//   Found    a background display flag, for a search result
//   Bad      a background display flag, e.g. for a mismatched bracket
//
// For display purposes, a delimiter or bracket type is replaced by the type in
// the first 26 starting with the same letter. The one-letter types are
// otherwise unused. That leaves room for the flags. The Bad flag is added to
// mismatched brackets, unclosed quotes, and Wrong tokens. The other flags are
// for use by other modules.

enum type {
    Alternative, B, Comment, Declaration, E, Function, G, H, Identifier, Join,
    Keyword, Long, Mark, Note, Operator, Property, Quote, R, S, Tag, Unary,
    Value, Wrong, X, Y, Z, None, Gap, Indent,

    NoteD, QuoteD,

    LongB, CommentB, CommentNB, TagB, RoundB, Round2B, SquareB, Square2B,
    GroupB, Group2B, BlockB, Block2B,

    LongE, CommentE, CommentNE, TagE, RoundE, Round2E, SquareE, Square2E,
    GroupE, Group2E, BlockE, Block2E,

    FirstB = LongB, LastB = Block2B, FirstE = LongE, LastE = Block2E,

    Cursor = 32, Select = 64, Found = 128, Bad = 64 + 128,
};

// Create a new tokens object, initially empty.
Tokens *newTokens(byte *types);

// TODO: type functions.

// Delete n token bytes before the gap.
void deleteTokens(Tokens *ts, int n);

// Insert n token bytes at the start of the gap, and prepare for scanning.
void insertTokens(Tokens *ts, int n);

// Check if the top of the unmatched opener stack matches a closer type.
bool matchTop(Tokens *ts, int type);

// Give token at t the given type, and do any appropriate bracket matching.
void setToken(Tokens *ts, int t, int type);

// TODO: Move cursor.
// TODO: Read for display.
