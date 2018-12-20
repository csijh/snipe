// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// A style indicates the type of a token, and is associated in a theme with a
// (background or foreground) colour for syntax highlighting. To reduce the
// number of types used in language definitions and themes, the default colour
// for a style is the colour for the preceding style. The Skip and More styles
// are used in the scanner. The Point and Select styles represent zero-length
// tokens added to mark the text caret or to toggle selection highlighting.
enum style {
    Point,        // Text caret
    Select,       // Toggle selection; specifies background colour
    Gap,          // Spaces; specifies overall background colour of display
    Word,         // Default text
    Name,         // Word forming a name
    Id,           // Word forming an identifier
    Variable,     // Word forming a variable name
    Field,        // Word forming a field name, e.g. following '.'
    Function,     // Word forming a function name, e.g. followed by '('
    Key,          // Keyword
    Reserved,     // Keyword forming a reserved word
    Property,     // Keyword forming a property name
    Type,         // Keyword forming a type name
    BadSign,      // Sign, incomplete
    Sign,         // Sign such as a key symbol or a punctuation mark
    Label,        // Sign such as a colon indicatiing a label
    BadOpen,      // Open bracket, unmatched or mismatched
    Open,         // Open bracket
    BadClose,     // Close bracket, unmatched or mismatched
    Close,        // Close bracket
    BadOp,        // Operator, incomplete or unrecognised
    Op,           // Operator
    BadNumber,    // Number, incomplete
    Number,       // Number, i.e. numerical literal
    BadChar,      // Character literal, incomplete
    Char,         // Character literal
    BadString,    // String, incomplete
    String,       // String literal
    BadParagraph, // Paragraph, incomplete
    Paragraph,    // Paragraph, i.e. multiline string
    BadEscape,    // Escape sequence, incomplete
    Escape,       // Escape sequence
    BadComment,   // Comment, incomplete
    OpenComment,  // Comment, multiline, not nested, start of
    CloseComment, // Comment, multiline, not nested, end of
    BadNest,      // Comment, nested, incomplete
    OpenNest,     // Comment, nested, start of
    CloseNest,    // Comment, nested, end of
    OpenNote,     // One-line comment, start
    CloseNote,    // One-line comment, end
    BadText,      // Illegal character or token
    CountStyles
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
