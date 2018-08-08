// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// A style indicates a property of a text byte such as its token type and,
// except for START, is associated in a theme with a background or foreground
// colour for syntax highlighting. A style byte can contain any of the first
// three styles as flags, plus one of the other styles. Each non-flag style
// has a letter, which is upper case for the start of a token, otherwise lower.
enum style {
    START,        // flag: first text byte of a token
    POINT,        // flag: preceded by text caret; colour of caret
    SELECT,       // flag: selected character; background colour for selections
    GAP,          // G space; default background colour
    WORD,         // W default text
    NAME,         // M word forming a name
    ID,           // I identifier
    VARIABLE,     // V variable name
    FIELD,        // D field name, e.g. following '.'
    FUNCTION,     // F function name, e.g. followed by '('
    KEY,          // K keyword
    RESERVED,     // R reserved word
    PROPERTY,     // P property
    TYPE,         // T type name
    SIGN,         // X key symbol or punctuation
    LABEL,        // L colon or other indication of a label
    OP,           // O operator
    NUMBER,       // N numerical literal
    STRING,       // S quoted string literal
    CHAR,         // C quoted character literal
    COMMENT,      // Z multi-line comment
    NOTE,         // Y one-line comment
    BAD,          // B illegal token
    COUNT_STYLES = BAD + 1
};
typedef unsigned char style;
typedef unsigned char compoundStyle;

// Find a style constant from its name or unique abbreviation.
style findStyle(char *name);

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
