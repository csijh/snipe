// Token and grapheme scanner. Free and open source. See licence.txt.

// Each byte of text in a document is given a corresponding tag byte. The tag
// for the first byte of a token gives its classification (for word-based cursor
// movement, syntax highlighting and bracket matching) and the tags for
// continuation bytes indicate the start of graphemes (the unit of ordinary
// cursor movement). Tags are generated using a language description state
// machine loaded from the languages folder.

// Each tag consists of a 6-bit value and 2 flag bits. The value gives the type
// of a token, or marks the byte as a continuation byte, picking out the first
// byte of each grapheme. The flag bits apply to a bracket token to indicate
// matching status, or to a newline to indicate whether the next line starts
// within a comment or literal.

// Tags are visualized as ASCII characters in language description files and
// themes, and when tracing and testing. Upper case letters are used to classify
// tokens which only affect highlighting. Their effects are determined entirely
// by language description files and theme files. Tags visualized as non-letter
// symbols have syntactic significance, and can affect cursor movement, bracket
// matching, indenting, and semicolon handling.

enum tag {
    Byte,       // '.' Continuation byte in a grapheme
    Char,       // ' ' First byte of continuation character in a token
    Gap,        // '_' White space
    Operator,   // '+' Operator token, affects semicolon handling
    Label,      // ':' Label indicator, affects indenting
    Quote,      // "'" Single quote, open or close one-line literal
    Quotes,     // '"' Double quote, open or close one-line literal
    Note,       // '#' Open one-line comment, e.g. # or //
    OpenR,      // '(' Open round bracket
    CloseR,     // ')' Close round bracket
    OpenS,      // '[' Open square bracket
    CloseS,     // ']' Close square bracket
    OpenC,      // '{' Open curly bracket block (or 'begin')
    CloseC,     // '}' Close curly bracket block (or 'end')
    OpenI,      // '%' Open curly bracket initializer
    CloseI,     // '!' Close curly bracket initializer
    Comment,    // '<' Open multiline comment, e.g. /*
    EndComment, // '>' Close multiline comment, e.g. */
    Para,       // '^' Multiline literal delimiter, e.g. """
    Newline,    // '$' Newline, end of one-line comment
    Invalid,    // '?' Invalid token
    AToken,     // 'A' First of 26 capital letter tags for highlighting
    Commented = 0x80,   // Flag bits for newlines
    Quoted = 0x40,
    Matched = 0x80,     // flag bits for brackets and delimiters
    Unmatched = 0x40,
    Mismatched = 0xC0
};

char showTag(int tag);
