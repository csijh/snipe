// Token and grapheme scanner. Free and open source. See licence.txt.

// Each byte of text in a document is given a corresponding tag byte. These tags
// mark the start of graphemes (the unit of cursor movement), and the start of
// tokens (the unit of word-based cursor movement and syntax highlighting), and
// classify the tokens. Tags are generated using language description state
// machines loaded from the languages folder.

// Each tag consists of a 6-bit value and a 2-bit flag. The value gives the type
// of a token, or marks the byte as the first byte of a continuation grapheme,
// or a continuation byte within a grapheme. The flag overrides the token type
// to indicate that the token is within a comment, or between quotes, or is an
// unmatched bracket.

// Tags are visualized as ASCII characters in language description files and
// themes, and when tracing and testing. Upper case letters are used to classify
// tokens which only affect highlighting. Their effects are determined entirely
// by language description files and theme files. Tags visualized as non-letter
// symbols have syntactic significance, and can affect cursor movement, bracket
// matching, indenting, and semicolon handling.

enum tag {
    Byte,       // '.' Continuation byte in a grapheme
    Grapheme,   // ' ' First byte of continuation grapheme in a token
    Gap,        // '_' White space
    Operator,   // '+' Operator token, affects semicolon handling
    Label,      // ':' Label indicator, affects indenting
    Quote,      // "'" Single quote, open or close literal
    Quotes,     // '"' Double quote, open or close literal
    OpenR,      // '(' Open round bracket
    CloseR,     // ')' Close round bracket
    OpenS,      // '[' Open square bracket
    CloseS,     // ']' Close square bracket
    OpenC,      // '{' Open curly bracket block (or 'begin')
    CloseC,     // '}' Close curly bracket block (or 'end')
    OpenI,      // '%' Open curly bracket initializer
    CloseI,     // '|' Close curly bracket initializer
    Comment,    // '<' Open multiline comment, e.g. /*
    EndComment, // '>' Close multiline comment, e.g. */
    Note,       // '#' Open one-line comment, e.g. # or //
    Newline,    // '\n' Newline, end of one-line comment
    Invalid,    // '?' Invalid token
    AToken,     // 'A' First of 26 tags representing capital letters
    Commented = 0x80,   // Flag bits overriding the main tag value
    Quoted = 0x40,
    Unmatched = 0xC0
};

char showTag(int tag);
