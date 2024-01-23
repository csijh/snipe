// The Snipe editor is free and open source. See licence.txt.
#include <stdbool.h>

// Each byte of source text has a corresponding style byte. The first 26 styles
// are token types (with some unused). The next 5 are background and caret
// indicators, so that the first 31 styles are indexes into a theme. Extended
// bracket styles for bracket matching are in matching pairs, with a B, 2B, E
// or 2E suffix. They can be reduced to Quote, Comment, Tag or Bracket for
// display. The Bad flag is added to mismatched or unmatched brackets. The
// First flag is added to the first byte of a token.
enum style {
    Alternative, Bracket, Comment, Declaration, Error, Function, Gap, H,
    Identifier, J, Keyword, L, Mark, N, Operator, Property, Quote, R, Sign,
    Tag, Unary, Value, W, X, Y, Z, Ground, Select, Focus, Warn, Caret,

    QuoteB, Quote2B, CommentB, Comment2B, TagB, RoundB, Round2B, SquareB,
    Square2B, GroupB, Group2B, BlockB, Block2B,

    QuoteE, Quote2E, CommentE, Comment2E, TagE, RoundE, Round2E, SquareE,
    Square2E, GroupE, Group2E, BlockE, Block2E,

    FirstB = QuoteB, LastB = Block2B, FirstE = QuoteE, LastE = Block2E,

    Bad = 64, First = 128, Flags = Bad | First
};

// A style can be stored in a byte.
typedef unsigned char byte;

// Return the full name of a style without flags.
char *styleName(int style);

// Find a style from its name or unique prefix, or return -1.
int findStyle(char const *name);

// For visualization purposes, translate brackets to Quote, Comment, Tag or
// Bracket and return the first letter of the style name. Return the letter in
// lower case if the Bad flag is set. Return a continuation byte of a token as
// a minus sign, and return Gap as a space.
char visualStyle(int style);

// Check for an opening bracket style, i.e. between FirstB and LastB.
bool isOpener(int style);

// Check for a closing bracket style, i.e. between FirstE and LastE.
bool isCloser(int style);

// Check for a bracket, i.e. between FirstB and LastE.
bool isBracket(int style);

// Check whether opening and closing brackets match.
bool bracketMatch(int opener, int closer);

// Return whether a style represents a prefix or infix token, preventing a
// following auto-inserted semicolon.
bool isPrefix(int style);

// Return whether a style represents a postfix or infix token, preventing a
// preceding auto-inserted semicolon.
bool isPostfix(int style);

// Prepare a style for display. Convert extended bracket styles to Quote,
// Comment, Tag or Bracket to provide 5 bits of foreground style. Remove the
// First flag. Convert the Bad flag to the Warn background style.
int displayStyle(int style);

// Add a background, one of Ground, Select, Focus, Warn to a style as 2 bits
// (unless Warn is already set), e.g. to indicate a selection.
int addBackground(int style, int bg);

// Add Caret as a 1-bit flag to a style to indicate that the byte should be
// preceded by a caret.
int addCaret(int style);

// Get the foreground, background, or caret flag from a style
int foreground(int style);
int background(int style);
bool hasCaret(int style);
