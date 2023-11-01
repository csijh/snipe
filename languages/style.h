// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// A style indicates the type of a token, and is associated in a theme with a
// (background or foreground) colour for syntax highlighting. To reduce the
// number of styles used in language definitions and themes, the default colour
// for a style is the colour for the preceding style. The Point and Select
// styles represent zero-length tokens added to mark the text caret or to toggle
// selection highlighting. For every style constant such as Key there is a style
// Bad+Key with name "BadKey" to represent an incomplete or malformed token.
// Any changes here must be matched by changes in the array of names in style.c
enum style {
    More,         // Continuation of current token
    Text,         // Characaters not part of any normal token
    Point,        // Text caret
    Select,       // Toggle selection; specifies background colour
    Gap,          // Spaces; specifies overall background colour of display
    Word,         // Word starting with a letter
    Name,         // Word forming a name
    Id,           // Word forming an identifier
    Variable,     // Word forming a variable name
    Field,        // Word forming a field name, e.g. following '.'
    Function,     // Word forming a function name, e.g. followed by '('
    Key,          // Keyword
    Reserved,     // Keyword forming a reserved word
    Property,     // Keyword forming a property name
    Type,         // Keyword forming a type name
    Sign,         // Sign such as a key symbol or a punctuation mark
    Label,        // Sign such as a colon indicatiing a label
    Open,         // Open bracket
    Close,        // Close bracket
    Op,           // Operator
    Number,       // Number, i.e. numerical literal
    Char,         // Character literal
    String,       // String literal
    Paragraph,    // Paragraph, i.e. multiline string
    Escape,       // Escape sequence
    OpenComment,  // Comment, multiline, not nested, start of
    CloseComment, // Comment, multiline, not nested, end of
    OpenNest,     // Comment, nested, start of
    CloseNest,    // Comment, nested, end of
    OpenNote,     // One-line comment, start
    CloseNote,    // One-line comment, end
    Bad,          // Flag to add to a style
    CountStyles = 2 * Bad
};
typedef unsigned char style;

// Find a style constant from its name. The name can be prefixed with Bad.
style findStyle(char *name);

// Find a style name from its constant. The constant can Bad+Style.
char *styleName(style s);
