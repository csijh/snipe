// The Snipe editor is free and open source. See licence.txt.
#include <stdbool.h>

// Each byte of source text has a corresponding byte containing a kind. The
// first 26 kinds are token types (with some unused). The Gap kind marks a
// space or newline or indent as a separator token. The More kind marks token
// bytes after the first. The kinds Ground...Caret represent background styles,
// so that kinds up to Caret can be used as indexes into a theme. Bracket kinds
// have matching pairs, with a B, 2B, E or 2E suffix. The Bad kind is used as a
// removable flag for mismatched or unmatched brackets.
enum kind {
    Alternative, Bracket, Comment, Declaration, Error, Function, Gap, H,
    Identifier, J, Keyword, L, Mark, N, Operator, Property, Quote, R, Sign,
    Tag, Unary, Value, W, X, Y, Z, More,
    Ground, Select, Focus, Warn, Caret,

    QuoteB, Quote2B, CommentB, Comment2B, TagB, RoundB, Round2B, SquareB,
    Square2B, GroupB, Group2B, BlockB, Block2B,

    QuoteE, Quote2E, CommentE, Comment2E, TagE, RoundE, Round2E, SquareE,
    Square2E, GroupE, Group2E, BlockE, Block2E,

    FirstB = QuoteB, LastB = Block2B, FirstE = QuoteE, LastE = Block2E,
    Bad = 128
};

// Return the full name of the kind.
char *kindName(int kind);

// Find a kind from its name or unique prefix, or return -1.
int findKind(char const *name);

// For visualization purposes, translate brackets to Quote, Comment or Bracket,
// then return the first letter of the kind name. Return it in lower case if it
// is a mismatched bracket. Return More as a minus sign, and Gap as a space.
char visualKind(int kind);

// Check for an opening bracket kind, i.e. between FirstB and LastB.
bool isOpener(int kind);

// Check for a closing bracket kind, i.e. between FirstE and LastE.
bool isCloser(int kind);

// Check for a bracket, i.e. between FirstB and LastE.
bool isBracket(int kind);

// Check whether opening and closing brackets match.
bool bracketMatch(int opener, int closer);

// Return whether a kind represents a prefix or infix token, preventing a
// following auto-inserted semicolon.
bool isPrefix(int kind);

// Return whether a kind represents a postfix or infix token, preventing a
// preceding auto-inserted semicolon.
bool isPostfix(int kind);

// A style is a byte derived from a kind for display purposes. The bracket types
// with B or E or 2B or 2E suffixes are replaced by Quote, Comment, Tag or
// Bracket, to provide 5 bits of foreground information. Two bits indicate a
// background (Ground, ..., Warn). The Bad flag is converted into a Warn
// background. A final bit indicates the presence of a caret.
typedef unsigned char byte;

// Get a style from a kind, converting bracket types and adding the Warn
// background for the Error kind or the Bad flag.
byte toStyle(int kind);

// Add a background indication to a style (unless Warn is already set).
byte addBackground(byte s, int kind);

// Add a caret flag to a style.
byte addCaret(byte s);

// Get the foreground, background, or caret flag from a style
int foreground(byte s);
int background(byte s);
bool hasCaret(byte s);

