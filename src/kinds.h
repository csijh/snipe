// Snipe editor. Free and open source, see licence.txt.
#include <stdbool.h>

// Each byte of source text has a corresponding byte containing a kind. The
// first byte of a token is taggeded with the token's type, and the remaining
// bytes are tagged with None. Special tags are:
//
//   None     marks token bytes after the first
//   Gap      marks a space or newline or indent as a separator
//   Bad      a flag for a mismatched bracket, unclosed quote, or illegal token
//
// Bracket kindss come in matching pairs, with a B or E suffix.

enum kind {
    None, Gap, Alternative, Block, Comment, Declaration, Error, Function, Group,
    H, Identifier, Jot, Keyword, L, Mark, Note, Operator, Property, Quote,
    Round, Square, Tag, Unary, Value, W, X, Y, Z,

    QuoteB, Quote2B, CommentB, Comment2B, TagB, RoundB, Round2B, SquareB,
    Square2B, GroupB, Group2B, BlockB, Block2B,

    QuoteE, Quote2E, CommentE, Comment2E, TagE, RoundE, Round2E, SquareE,
    Square2E, GroupE, Group2E, BlockE, Block2E,

    FirstB = QuoteB, LastB = Block2B, FirstE = QuoteE, LastE = Block2E,

    Bad = 128,
};

typedef unsigned char Kind;

// Return the full name of the kind.
char *kindName(Kind k);

// For display purposes, return a compact 5 bit version of a kind. A bracket
// kind is replaced by the version without the B, 2B, E, 2E suffix. Mismatched
// brackets are returned as Error.
Kind displayKind(Kind k);

// For visualization purposes, return the first letter of the kind name. Return
// it in lower case if it is a mismatched bracket. Return None as a minus sign,
// and Gap as a space.
char visualKind(Kind k);

// Check for an opening bracket kind, i.e. between FirstB and LastB.
bool isOpener(Kind k);

// Check for a closing bracket kind, i.e. between FirstE and LastE.
bool isCloser(Kind k);

// Check for a bracket, i.e. between FirstB and LastE.
bool isBracket(Kind k);

// Check whether opening and closing brackets match.
bool bracketMatch(Kind opener, Kind closer);

// Return whether a kind represents a prefix or infix token, preventing a
// following auto-inserted semicolon.
bool isPrefix(Kind k);

// Return whether a kind represents a postfix or infix token, preventing a
// preceding auto-inserted semicolon.
bool isPostfix(Kind k);
