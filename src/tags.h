// Snipe token and grapheme scanner. Free and open source. See licence.txt.

// A tags object contains information about the tokens and graphemes in the
// text. Tokens are the units of 'word-based' cursor movement, and graphemes are
// the units of ordinary left/right cursor movement. A grapheme is a Unicode
// character beyond ASCII, or a combining sequence of Unicode characters forming
// a single visual unit. Tags can be used for auto bracket matching, indenting,
// semicolons and wrapping without accessing the original text. The tag bytes
// are kept in a gap buffer which closely resembles the one used for the text.
struct tags;
typedef struct tags tags;

// A tag is held in an unsigned byte.
typedef unsigned char tag;

// A tag value is one of these constants. For each tag, there is a short name
// consisting of one ASCII character for ease of testing, visualisation and
// table construction. This is a symbol for tags with more significance than
// just syntax highlighting, and otherwise it is an upper case letter.  A tag
// byte can accommodate flag bits to describe overriding. The original value can
// be overridden by COMMENTED, QUOTED or BAD.
enum tag {
    SKIP,           // ~ Continuation of character (grapheme)
    MORE,           // - Continuation of token
    GAP,            // _ Space
    BAD,            // ? Invalid/incomplete/mismatched token
    LEFT0,          // ( Open round bracket
    RIGHT0,         // ) Close round bracket
    LEFT1,          // [ Open square bracket
    RIGHT1,         // ] Close square bracket
    LEFT2,          // { Open curly bracket or similar
    RIGHT2,         // } Close curly bracket or similar
    COMMENT,        // # One-line comment
    COMMENT0,       // < Open delimiter, non-nesting multiline comment
    COMMENT1,       // > Close delimiter
    COMMENT2,       // ^ Open delimiter, nesting multiline comment
    COMMENT3,       // $ Close delimiter
    COMMENTED,      // * Text inside (any kind of) comment
    QUOTE,          // ' Single quote
    DOUBLE,         // " Double quote
    TRIPLE,         // @ Multiline quote (often """)
    QUOTED,         // = Text inside (any kind of) quotes
    NEWLINE,        // . End of line, treated as a token
    LABEL,          // : Label indicator, often colon

    ESCAPE,         // E Escape sequence
    IDENTIFIER,     // I Identifier
    TYPE,           // T Alternative id
    FUNCTION,       // F Alternative id
    PROPERTY,       // P Alternative id
    KEYWORD,        // K Keyword (or key symbol)
    RESERVED,       // R Alternative keyword (or named constant)
    VALUE,          // V Numeric or other literal
    OPERATOR,       // O Unary or binary operator
    SIGN,           // S Symbol or punctuation
    TAG_COUNT
};

// Create a tags object from a state machine table for a default language. Text
// is scanned using the table and a generic scanner.
tags *newTags(void *table);

// Free up a tags object.
void freeTags(tags *ts);

// Find a tag from its long or short name, or return 255.
tag findTag(char *name);

// Find a tag's full or short name.
char const *longTagName(tag t);
char shortTagName(tag t);

// Get tag at position p. If tag is overridden, return the override value. If it
// is JOIN, return the same tag value as the first byte of its token.
tag getTag(tags *ts, int p);

// Override tag value with COMMENTED, QUOTED or BAD, or remove override.
void override(tags *ts, int p, tag o);
void unoverride(tags *ts, int p);

// Copy tags for the n text bytes at position p into the array a. Replace
// overridden tags.
// void getTags(tags *ts, int p, int n, char *a);

// Change languages by providing a replacement state machine table.
void changeLanguage(tags *ts, void *table);

// Get position of next token after p, or previous token before p.
int nextToken(tags *ts, int p);
int backToken(tags *ts, int p);

// Get position of next grapheme after p, or previous grapheme before p.
int nextGrapheme(tags *ts, int p);
int backGrapheme(tags *ts, int p);
