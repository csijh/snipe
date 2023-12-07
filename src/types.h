// Snipe editor. Free and open source, see licence.txt.
#include <stdbool.h>

// Each byte of source text has a corresponding byte containing a type. The
// first byte of a token is marked with the token's type, and the remaining
// bytes are marked with None. Special types are:
//
//   None     marks token bytes after the first
//   Gap      marks a space or newline or indent as a separator
//   Bad      a flag for a mismatched bracket, unclosed quote, or illegal token
//
// Bracket types come in matching pairs, with a B or E suffix.

enum type {
    Alternative, B, Comment, Declaration, E, Function, G, H, Identifier, Jot,
    Keyword, L, Mark, Note, Operator, Property, Quote, R, S, Tag, Unary,
    Value, Wrong, X, Y, Z, None, Gap,

    QuoteB, Quote2B, CommentB, Comment2B, TagB, RoundB, Round2B, SquareB,
    Square2B, GroupB, Group2B, BlockB, Block2B,

    QuoteE, Quote2E, CommentE, Comment2E, TagE, RoundE, Round2E, SquareE,
    Square2E, GroupE, Group2E, BlockE, Block2E,

    FirstB = QuoteB, LastB = Block2B, FirstE = QuoteE, LastE = Block2E,

    Bad = 128,
};

typedef unsigned char byte;

// Return the full name of the type.
char *typeName(int type);

// For display purposes, return a compact 5 bit version of a type. A bracket
// type is replaced by the type in the first 26 starting with the same letter.
// (The one-letter types are otherwise unused.) The Bad flag is retained.
byte displayType(int type);

// For visualization purposes, return the first letter of the type name. Return
// it in lower case if the type has the Bad flag set. Return None as a minus
// sign and Gap as a space.
char visualType(int type);

// Check for an opening bracket type, i.e. between FirstB and LastB.
bool isOpener(int type);

// Check for a closing bracket type, i.e. between FirstE and LastE.
bool isCloser(int type);

// Check whether opening and closing brackets match.
bool bracketMatch(int opener, int closer);

// Return whether a type represents a prefix or infix token, preventing a
// following auto-inserted semicolon.
bool isPrefix(int type);

// Return whether a type represents a postfix or infix token, preventing a
// preceding auto-inserted semicolon.
bool isPostfix(int type);
