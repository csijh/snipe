// Snipe editor. Free and open source, see licence.txt.
#include <stdbool.h>

// Each byte of source text has a corresponding byte containing a kind. The
// first 26 kinds are token types. The Gap kind marks a space or newline or
// indent as a separator. The More kind marks token bytes after the first. The
// Normal, Select, Focus, Warn, Caret types specify background or caret styles,
// so that the kinds up to Caret can be used to index themes. Bracket kinds
// come in matching pairs, with a B or E suffix. Bad is used as a reversible
// flag for mismatched or unmatched brackets.
enum kind {
    Alternative, Block, Comment, Declaration, Error, Function, Group,
    H, Identifier, Jot, Keyword, L, Mark, Note, Operator, Property, Quote,
    Round, Square, Tag, Unary, Value, W, X, Y, Z,
    Gap, Normal = Gap, More, Select = More, Focus, Warn, Caret,

    QuoteB, Quote2B, CommentB, Comment2B, TagB, RoundB, Round2B, SquareB,
    Square2B, GroupB, Group2B, BlockB, Block2B,

    QuoteE, Quote2E, CommentE, Comment2E, TagE, RoundE, Round2E, SquareE,
    Square2E, GroupE, Group2E, BlockE, Block2E,

    FirstB = QuoteB, LastB = Block2B, FirstE = QuoteE, LastE = Block2E,
    Bad = 128
};
typedef unsigned char Kind;

// Return the full name of the kind.
char *kindName(Kind k);

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

// A style is a byte derived from a kind for display purposes. The bracket types
// with B or E or 2B or 2E suffixes are replaced by the versions without
// suffixes, to provide 5 bits of foreground information. Two bits indicate a
// background (Normal, ..., Warn). The Bad flag is converted into a Warn
// background. A final bit indicates the presence of a caret.
typedef unsigned char Style;

// Convert a kind to a style, converting bracket types and the Bad flag.
Style toStyle(Kind k);

// Add a background indication to a style (unless Warn is already set).
Style addBackground(Kind k);

// Add a caret flag to a style.
Style addCaret(Style s);

// Get the foreground, background, or caret flag from a style
Kind foreground(Style s);
Kind background(Style s);
bool hasCaret(Style s);

