// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// A style indicates a syntactic property of a text byte. The first byte of a
// token has a style from GAP onwards, possibly with BAD added in. Subsequent
// bytes have styles BYTE, POINT or GRAPH, indicating code point and extended
// grapheme cluster boundaries. A style is also used to specify colours in
// themes, with CURSOR and SELECT used for cursor and selection colours. Many
// styles have defaults, allowing them to be used to support syntactic editor
// features without needing a separate colour in themes. A style has a letter
// for testing.
enum style {
    CURSOR,     // c For associating a foreground colour with the cursor
    SELECT,     // s For associating a background colour with selections
    BYTE,       // b Continuation byte
    POINT,      // p Next code point, same token
    GRAPH,      // g Next grapheme, same token
    GAP,        // G space; display background colour
    WORD,       // W default text
    NAME,       // M word forming a name, default colour WORD
    ID,         // I identifier, default colour WORD
    VARIABLE,   // V variable name, default colour WORD
    FIELD,      // D field name, e.g. following '.', default colour WORD
    FUNCTION,   // F function name, e.g. followed by '(', default colour WORD
    KEY,        // K keyword
    RESERVED,   // R reserved word, default colour KEY
    PROPERTY,   // P property, default colour KEY
    TYPE,       // T type name, default colour KEY
    SIGN,       // X key symbol or punctuation
    LABEL,      // L colon or other indication of a label, default colour SIGN
    OP,         // O operator, default colour SIGN
    NUMBER,     // N numerical literal
    STRING,     // S quoted string literal
    CHAR,       // C quoted character literal, default colour STRING
    COMMENT,    // Z multi-line comment
    NOTE,       // Y one-line comment, default colour COMMENT
    BAD,        // B Flag added to any of the above, illegal token
    COUNT_STYLES,
};
typedef unsigned char style;

// Find a style constant from its name or unique abbreviation.
style findStyle(char *name);

// Find the default for a style, or return the style unchanged.
style styleDefault(style s);

// Find a style name from its constant.
char *styleName(style s);

// Find a letter for a style.
char styleLetter(style s);

// Add a BAD flag to a style.
style badStyle(style s);

// Remove a BAD flag from a style.
style goodStyle(style s);

// Check for a BAD flag on a style.
bool isBadStyle(style s);
