// Snipe editor. Free and open source, see licence.txt.
#include <stdbool.h>

// Token types, one per byte of text. Single-letter types are used by
// displayType() or reserved for future use:
//
//     None    marks bytes of a token after the first
//     Error   indicates an illegal, malformed or mismatched token
//     White   marks a space or newline or indent as a separator
//
// Bracket types come in matching pairs: openers have a B suffix, and closers
// have an E suffix. The Bad flag marks mismatched brackets.
enum type {
    None, Alternative, B, Comment, Declaration, Error, Function, G, H,
    Identifier, Jot, Keyword, L, Mark, Note, Operator, Property, Quote, R, S,
    Tag, Unary, Value, White, X, Y, Z,

    QuoteB, Quote2B, CommentB, Comment2B, TagB, RoundB, Round2B, SquareB,
    Square2B, GroupB, Group2B, BlockB, Block2B,

    QuoteE, Quote2E, CommentE, Comment2E, TagE, RoundE, Round2E, SquareE,
    Square2E, GroupE, Group2E, BlockE, Block2E,

    FirstB = QuoteB, LastB = Block2B, FirstE = QuoteE, LastE = Block2E,
    Bad = 128
};

typedef unsigned char byte;

// A tokens object records tokens in the text, and handles bracket matching.
// Insertions, deletions and cursor moves in the text are tracked.
typedef struct tokens Tokens;

// Create a new tokens object, initially empty.
Tokens *newTokens();

// Track an insertion of n (unscanned) text bytes after the cursor position.
void insertTokens(Tokens *ts, int n);

// Track a deletion of n text bytes before the cursor. Associated tokens are
// removed, and brackets may be re-highlighted as matched or mismatched.
void deleteTokens(Tokens *ts, int n);

// Track a cursor movement to position p. Brackets may be re-highlighted.
void moveTokens(Tokens *ts, int p);

// Add a token at position p. Update brackets as appropriate.
void addToken(Tokens *ts, int p, byte type);

// Add a closer, only if it matches the top opener, returning success.
bool tryToken(Tokens *ts, int p, byte type);

// Read n token bytes from position p into the given byte array, returning
// the possibly reallocated array.
byte *readTokens(Tokens *ts, int p, int n, byte *bs);

// Return the full name of a type.
char *typeName(int type);

// Return whether a type represents a prefix or infix token, preventing a
// following auto-inserted semicolon.
bool isPrefix(int type);

// Return whether a type represents a postfix or infix token, preventing a
// preceding auto-inserted semicolon.
bool isPostfix(int type);

// For display purposes, return a compact 5 bit version of a type. A bracket
// type is replaced by the type in 1..26 starting with the same letter.
// Mismatched brackets are returned as Error.
byte displayType(int type);

// For visualization purposes, return the first letter of the type name. Return
// it in lower case if it is a mismatched bracket. Return None as a minus sign,
// and White as a space.
char visualType(int type);
