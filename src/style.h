// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// A style indicates a syntactic property of a text byte. A style byte can
// contain any of the first three styles as flags, plus one of the other styles.
// For syntax highlighting, any style except the START flag can be associated in
// a theme with a background or foreground colour. Many styles have defaults,
// allowing them to be used to support syntactic editor features without needing
// a separate colour. Each non-flag style has a letter for visualization or
// testing, which is upper case for the start of a token, otherwise lower.
enum style {
    START,        // flag: first text byte of a token
    POINT,        // flag: character preceded by text caret; colour of caret
    SELECT,       // flag: selected character; background colour for selections
    GAP,          // G space; display background colour
    WORD,         // W default text
    NAME,         // M word forming a name, default colour WORD
    ID,           // I identifier, default colour WORD
    VARIABLE,     // V variable name, default colour WORD
    FIELD,        // D field name, e.g. following '.', default colour WORD
    FUNCTION,     // F function name, e.g. followed by '(', default colour WORD
    KEY,          // K keyword
    RESERVED,     // R reserved word, default colour KEY
    PROPERTY,     // P property, default colour KEY
    TYPE,         // T type name, default colour KEY
    SIGN,         // X key symbol or punctuation
    LABEL,        // L colon or other indication of a label, default colour SIGN
    OP,           // O operator, , default colour SIGN
    NUMBER,       // N numerical literal
    STRING,       // S quoted string literal
    CHAR,         // C quoted character literal, default colour STRING
    COMMENT,      // Z multi-line comment
    NOTE,         // Y one-line comment, default colour COMMENT
    BAD,          // B illegal token
    COUNT_STYLES
};
typedef unsigned char style;
typedef unsigned char compoundStyle;

// Find a style constant from its name or unique abbreviation.
style findStyle(char *name);

// Find the default for a style, or return the style unchanged.
style styleDefault(style s);

// Find a style name from its constant.
char *styleName(style s);

// Find a letter for a style (upper case if it has the START flag).
char styleLetter(style s);

// Add a flag to a style.
compoundStyle addStyleFlag(style s, style flag);

// Check for a flag.
bool hasStyleFlag(compoundStyle s, style flag);

// Take off any flags.
style clearStyleFlags(compoundStyle s);
