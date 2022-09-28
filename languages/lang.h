// Specific language modules include this header, as well as lang.c

// A language is based on token tags, which encode information for incremental
// scanning, word motion, bracket matching, indenting, and semicolon handling.
// The last three values can be added to any other tag, to make a reversible
// change during bracket matching.
enum Tag {
    Bad,        // malformed token
    Warn,       // bad fragment of comment or quote
    Gap,        // sequence of spaces
    Note,       // fragment of one-line comment
    Quote,      // fragment of quote, i.e. '...' or "..." literal
    Value,      // number or similar
    Type,       // type-related keyword
    Key,        // keyword
    Reserved,   // alternative kind of keyword
    Id,         // identifier
    Function,   // alternative kind of identifier
    Property,   // alternative kind of identifier
    Newline,    // active newline
    Endline,    // inactive newline, inside long comment or long quote
    PreOp,      // prefix operator
    InOp,       // infix operator
    PostOp,     // postfix operator
    PreInOp,    // prefix-or-infix operator, e.g. minus (resolved by context)
    PrePostOp,  // prefix-or-postfix operator, e.g. increment (ditto)
    Sign,       // punctuation or similar
    PreSign,    // prefix sign
    InSign,     // infix sign
    PostSign,   // postfix sign
    Open0,      // open bracket, level 0, e.g. (
    Close0,     // close bracket, level 0, e.g. )
    Open1,      // open bracket, level 1, e.g. [
    Close1,     // close bracket, level 1, e.g. ]
    Open2,      // open bracket, level 2, e.g. { used as initialiser
    Close2,     // close bracket, level 2 e.g. } which needs a semicolon
    OpenB,      // open block, e.g. { used as block bracket
    CloseB,     // close block, e.g. } with no semicolon
    OpenC,      // open multiline comment (non-nesting)
    CloseC,     // close multiline comment (non-nesting)
    OpenQ,      // open multiline quote
    CloseQ,     // close multiline quote
    Commented = 0x80,  // added to other tokens to reversibly comment them out
    Quoted = 0x40,     // added to other tokens to reversibly quote them
    Mismatched = 0xC0, // added to other tokens to reversibly mark as errors
};

// The Tag type is used when a tag is stored compactly (otherwise int).
typedef unsigned char Tag;

// A token is a tag and a length. The maximum length is 255. A token longer than
// this is unlikely, since comments and quotes are divided into fragments and
// multi-line constructs are handled by bracket matching. But if a long token
// does occur, it can be broken into fragments. If a token has length 0, the tag
// contains language-specific scan-state information.
struct token { Tag tag, length; };
typedef struct token token;

// Languages that are currently supported.
enum language { C };

// The scan function provided by lang.c. The arguments passed in are the current
// language, the tag from the last token of the previous line to act as a scan
// state, a line of text terminated by newline. The function produces tokens in
// the out array. It adds a zero length token at the end, with the scan state as
// the tag. The array is guaranteed to be long enough (length of line plus one).
void scan(int lang, int state, char const *s, token *out);

// The scan function for C, provided by lang-c.c
void scanC(int state, char const *s, token *out);

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
