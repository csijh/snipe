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

// A tag value is one of these constants. There is a one-letter version of each,
// for ease of testing and visualisation. The tag values are less than 32, so a
// tag byte can accommodate three flag bits, e.g. to describe overriding. The
// original value can be overridden by COMMENTED, QUOTED or BAD.
enum tag {
    GAP,       G = GAP,        // Space
    ROUND0,    R = ROUND0,     // Open round bracket
    ROUND1,    r = ROUND1,     // Close round bracket
    ANGLE0,    A = ANGLE0,     // Open square bracket
    ANGLE1,    a = ANGLE1,     // Close square bracket
    WAVY0,     W = WAVY0,      // Open curly bracket or similar
    WAVY1,     w = WAVY1,      // Close curly bracket or similar
    COMMENT,   C = COMMENT,    // One-line comment
    COMMENT0,  X = COMMENT0,   // Open delimiter, non-nesting multiline comment
    COMMENT1,  x = COMMENT1,   // Close delimiter
    COMMENT2,  Y = COMMENT2,   // Open delimiter, nesting multiline comment
    COMMENT3,  y = COMMENT3,   // Close delimiter
    COMMENTED, c = COMMENTED,  // Text inside (any kind of) comment
    QUOTE,     Q = QUOTE,      // Single quote
    DOUBLE,    D = DOUBLE,     // Double quote
    TRIPLE,    T = TRIPLE,     // Multiline quote (often """)
    QUOTED,    q = QUOTED,     // Text inside (any kind of) quotes
    NEWLINE,   N = NEWLINE,    // End of line, treated as a token
    HANDLE,    H = HANDLE,     // Label indicator, often colon
    ESCAPE,    E = ESCAPE,     // Escape sequence
    ID,        I = ID,         // Identifier
    ID1,       i = ID,         // Alternative id (e.g. type name)
    FUNCTION,  F = FUNCTION,   // Alternative id
    PROPERTY,  P = PROPERTY,   // Alternative id
    KEY,       K = KEY,        // Keyword (or key symbol)
    KEY1,      k = KEY1,       // Alternative keyword (e.g. named constant)
    VALUE,     V = VALUE,      // Numeric or other literal
    OPERATOR,  O = OPERATOR,   // Unary or binary operator
    SIGN,      S = SIGN,       // Symbol or punctuation
    BAD,       B = BAD,        // Invalid/incomplete token
    JOIN,      J = JOIN,       // Continuation of token
    MISS,      M = MISS        // Continuation of grapheme
};

// Create a tags object from a state machine table for a default language. Text
// is scanned using the table and a generic scanner.
tags *newTags(void *table);

// Free up a tags object.
void freeTags(tags *ts);

// Find a tag from its long or short name. Find a tag's full or one-letter name.
tag findTag(char *name);
char *longTagName(tag t);
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
