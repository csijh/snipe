// Snipe editor. Free and open source, see licence.txt.
#include <stdbool.h>

// Token types. One-letter ones are reserved for future use. Special ones are:
//
//     None    marks bytes of a token after the first
//     Error   indicates an illegal, malformed or mismatched token
//     White   marks a space or newline or indent as a separator
//
// Bracket types come in matching pairs: openers have a B suffix, and closers
// have an E suffix. The Block, Group, Round, Square types are the reduced
// forms of BlockB etc. used for display purposes. The Bad flag marks
// mismatched brackets.
enum type {
    None, Alternative, Block, Comment, Declaration, Error, Function, Group, H,
    Identifier, Jot, Keyword, L, Mark, Note, Operator, Property, Quote, Round,
    Square, Tag, Unary, Value, White, X, Y, Z,

    QuoteB, Quote2B, CommentB, Comment2B, TagB, RoundB, Round2B, SquareB,
    Square2B, GroupB, Group2B, BlockB, Block2B,

    QuoteE, Quote2E, CommentE, Comment2E, TagE, RoundE, Round2E, SquareE,
    Square2E, GroupE, Group2E, BlockE, Block2E,

    FirstB = QuoteB, LastB = Block2B, FirstE = QuoteE, LastE = Block2E,
    Bad = 128
};

typedef unsigned char byte;

// A tokens object records tokens in the text by storing one type byte per text
// byte, and handles bracket matching. After an edit to the text, a whole line
// needs to be rescanned. The corresponding operations on the tokens object are
// to delete a line of tokens, then insert a new unscanned line, then report
// the tokens found by the scanner one by one, then move the cursor back to its
// position in the line. Other cursor movements are tracked, because they
// affect bracket matching and indenting.
typedef struct tokens Tokens;

// Create a new tokens object, initially empty.
Tokens *newTokens();

// Delete a line of n type bytes before the cursor. Brackets may be
// re-highlighted as matched or mismatched.
void deleteLine(Tokens *ts, int n);

// Insert a line of n type bytes after the cursor, initially with no tokens.
void insertLine(Tokens *ts, int n);

// Add a token at position p. Update brackets as appropriate.
void addToken(Tokens *ts, int p, byte type);

// Check whether a closer matches the top opener. (This is what makes it
// necessary to keep brackets up to date with each token, rather than dealing
// with them at the end of the line.)
bool matchTop(Tokens *ts, byte type);

// -----------------------
// TODO: after adding tokens for a line, ask for the # of closers/openers.

// Move the cursor to position p. Brackets may be re-highlighted.
void moveTokens(Tokens *ts, int p);

// Read n token bytes from position p into the given byte array, which is
// assumed to have enough capacity.
void readTokens(Tokens *ts, int p, int n, byte *bs);

// Return the full name of a type.
char *typeName(int type);

// Return whether a type represents a prefix or infix token, preventing a
// following auto-inserted semicolon.
bool isPrefix(int type);

// Return whether a type represents a postfix or infix token, preventing a
// preceding auto-inserted semicolon.
bool isPostfix(int type);

// For display purposes, return a compact 5 bit version of a type. A bracket
// type is replaced by the version without the B, 2B, E, 2E suffix. Mismatched
// brackets are returned as Error.
byte displayType(int type);

// For visualization purposes, return the first letter of the type name. Return
// it in lower case if it is a mismatched bracket. Return None as a minus sign,
// and White as a space.
char visualType(int type);
