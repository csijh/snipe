// Snipe editor. Free and open source. See licence.txt.
#include <stdbool.h>

// Each specific language module includes this header. So does lang.c.

// Language handling is largely based on tags, which are integers in the range
// 0..255 and which are stored compactly in unsigned bytes.
typedef unsigned char byte;

// A token is a tag and a length. Comments and quotes are divided into fragment
// tokens, partly to support word motion and paragraph reformatting, and partly
// to support the handling of multi-line comments and quotes by bracket
// matching. The maximum length of a token is 255. A longer token, though very
// unlikely, is broken into fragments. A token can have length 0, for example to
// change background colour or specify a cursor position.
struct token { byte tag, length; };
typedef struct token token;

// Tags are used to specify styles for syntax highlighting, fixities for
// semicolon handling, and brackets for matching and indenting.
enum styleTag {

// These tags form the styles. Each tag beyond these maps to one of the
// Sign, Op, Quoted, or Commented styles.
    Gap,        // sequence of spaces (and the default background style)
    Mark,       // zero-length token to change background style for selections
    Warn,       // zero-length token to change background style for warnings
    Here,       // zero-length token for cursor (and its foreground style)
    Newline,    // newline (styled as Sign)
    Bad,        // malformed or misplaced token
    Commented,  // token inside comment
    Quoted,     // token inside quote
    Escape,     // escape sequence
    Sign,       // punctuation mark or similar (nonfix)
    Op,         // operator (nonfix)
    Value,      // number or similar
    Key,        // keyword
    Type,       // type or type-related keyword
    Reserved,   // alternative kind of keyword
    Id,         // identifier
    Property,   // alternative identifier, e.g. field name
    Function,   // alternative identifier, e.g. function name

// These tags allow signs and operators to be declared with given fixities. An
// operator such as + which can be prefix or infix is best declared as an InOp,
// i.e. infix, so that it always indicates a continuation line (with no
// semicolon) whether it is at the beginning or end of a line. An operator such
// as ++ which can be prefix or postfix is best declared as Op, i.e. nonfix, so
// that it never indicates a continuation line.
    PreSign,    // prefix sign
    InSign,     // infix sign
    PostSign,   // postfix sign
    PreOp,      // prefix operator
    InOp,       // infix operator
    PostOp,     // postfix operator

// These tags are brackets, which include delimiters for multi-line comments and
// quotes. The Begin tag is treated as infix because a block is assumed always
// to be attached to a preceding statement (which is especially relevant in the
// Allman indentation style). The End tag is treated as infix because no
// semicolon is needed after it. The OpenC, CloseC, BeginC, EndC tags are used
// for curly brackets, which can also be used as block brackets. The brackets {
// } can be tentatively tagged as BeginC, EndC, then { can be retagged as OpenC
// during context scanning if it is the start of an initialiser or declaration,
// and that causes } to be retagged as CloseC to match. The retagging of } is
// reversible, in case it later gets matched with a different { bracket.
    Open,       // open bracket, e.g. (
    Close,      // close bracket
    Open1,      // open bracket, level 1, e.g. [
    Close1,     // close bracket
    Open2,      // open bracket, level 2, e.g. { if not used for blocks
    Close2,     // close bracket
    Begin,      // begin block (infix)
    End,        // end block (infix)
    OpenC,      // matches CloseC; can change EndC to CloseC
    CloseC,     // close bracket which can be changed to EndC
    BeginC,     // matches EndC; can change CloseC to EndC
    EndC,       // end block which can be changed to CloseC
    Quote,      // single quote
    Misquote,   // missing quote at end of line (zero-length token)
    Quotes,     // double quote
    Misquotes,  // missing double quote at end of line (zero-length token)
    StartQ,     // start multi-line quote
    StopQ,      // stop multi-line quote
    Note,       // one-line comment
    StartC,     // start of multi-line comment
    StopC,      // stop multiline comment
};

// A language can defined further custom tags, provided they are temporary and
// are resolved to one of the above tags by the end of scanning.
enum { nTags = StopC+1 };

// Get the style of a tag (i.e. map to one of the first group of tags).
int style(int tag);

// Check if a tag is postfix (or infix) i.e. expects something on the right and
// so shouldn't be followed by a semicolon.
bool postfix(int tag);

// Check if a tag is prefix (or infix) i.e. expects something on the left and
// so shouldn't be preceded by a semicolon at the end of the previous line.
bool prefix(int tag);

// Provide a single-character name for a tag, for testing purposes.
char *nickname(int tag);

// Add a token to the end of the "out" string, so that it lines up with an input
// string. Add ' ' for a gap, and ~ otherwise to make up the length. Use an
// alternative name for the token, if given.
void showToken(token t, char *alt, char *out);

// Languages that are currently supported.
enum language { C };

// The scan function provided by lang.c. The arguments passed in are the current
// language, the scan state at the start of the line, a line of text terminated
// by a newline, and an array to put the tokens into The function returns the
// scan state at the end of the line (and therefore at the start of the next).
// The output array is guaranteed to be long enough (one token per char in s).
int scan(int lang, int state, char const *s, token *out);

// The scan functions for specific languages.
int scanC(int state, char const *s, token *out);

// TODO: Match brackets (forward/backward/left/right) By a line?

// TODO: repair indent and/or semicolon for a line on cursor move.

// TODO: stylize: convert a line of tokens into a line of styles.

/*
// A pair of tags passed to a matching function. For forward matching, the left
// tag belongs to the most recent unmatched opener, and the right tag belongs to
// the token which the cursor is about to be moved past. For backward matching,
// the right tag is a closer and the left tag is about to be moved past.
struct tagPair { int left, right; };
typedef struct tagPair tagPair;

// The result of matching: The tags are a pair (opener-closer), or the tags are
// the same type (opener-opener, or closer-closer), possibly with indenting
// being switched off (inside multiline comment or literal), or or one token is
// not to be treated as a bracket (opener-any or any-closer).
enum matching { Pair, Same, NonIndent, NonBracket };
typedef int matching;

// The type of a matching function. The function returns a matching constant,
// and may also change either of the two tags (reversibly).
typedef matching matchFunction(tagPair *pair);

// A language object. It provides the maximum number of chars of lookahead past
// the end of a token while scanning it (used to extend an edit leftwards before
// rescanning). It provides the maximum number of tokens of contextual lookback
// when determining a token's tag (used to provide a context for scanning, and
// as the number of identically scanned tokens needed to resynchronise). It
// provides a pointer to a scan function, and a pointer a style function. It
// provides functions to match or unmatch tokens, which are used both for
// forward and backward matching. It provides a fixity function.
struct language {
    scanFunction *scan;
    matchFunction *match;
    matchFunction *unmatch;
};
typedef struct language language;
*/
