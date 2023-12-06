// Snipe editor. Free and open source, see licence.txt.

// An array of types has one byte to mark each byte of text. The first byte of a
// token is marked with the token's type. Delimiters have a D suffix, and
// bracket types come in matching pairs, with a B or E suffix. Special types
// and flags are:
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
    Alternative, B, Comment, Declaration, E, Function, G, H, Identifier, Jot,
    Keyword, Long, Mark, Note, Operator, Property, Quote, R, S, Tag, Unary,
    Value, Wrong, X, Y, Z, None, Gap, Indent,

    NoteD, QuoteD,

    LongB, CommentB, Comment2B, TagB, RoundB, Round2B, SquareB, Square2B,
    GroupB, Group2B, BlockB, Block2B,

    LongE, CommentE, Comment2E, TagE, RoundE, Round2E, SquareE, Square2E,
    GroupE, Group2E, BlockE, Block2E,

    FirstB = LongB, LastB = Block2B, FirstE = LongE, LastE = Block2E,

    Cursor = 32, Select = 64, Found = 128, Bad = 64 + 128,
};

// Return the full name of the type.
char *typeName(int type);

// Return a compact (5 bit) version of a type, for display purposes, where a
// delimiter or bracket type is replaced by the type in the first 26 starting
// with the same letter. A bad flag on the type is retained.
char *compactType(int type);

// Return a letter representing a type. It is the first letter of the type name,
// but changed to lower case if the type has the Bad flag set. None is returned
// as a minus sign and Gap as a space.
char *letterType(int type);

// Return whether a type represents a prefix or infix token, preventing a
// following auto-inserted semicolon.
bool isPrefix(int type);

// Return whether a type represents a postfix or infix token, preventing a
// preceding auto-inserted semicolon.
bool isPostfix(int type);
