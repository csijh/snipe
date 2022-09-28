// Coordinate the available languages.

// Bracket types. Linear means non-bracket.
enum bracket { Linear, Open, Close };

// Bracket levels. Flat means non-bracket. Curly means curly brackets used as
// initialisers etc. (with semicolons). Block means curly brackets used for
// blocks (without semicolons). Delimiter means a multi-line comment delimiter.
enum level { Round, Square, Curly, Block, Delimiter, Flat };

// The information about each tag includes the scanner state for the next token,
// the fixity, the highlight style, the bracket type, and the bracket level.
struct tagInfo { char state, fixity, style, bracket, level; };
typedef struct tagInfo tagInfo;

// Tags are used to classify tokens. They encode and compress info for
// incremental scanning, word motion, bracket matching, indenting, and
// semicolons. They are in the range 0..255 so that they can be stored in an
// unsigned 8-bit int. The 'Commented' tag can be added to any other tag to
// reversibly comment it out inside a multiline comment.
enum tag {
    Gap, Note, NoteGap, NoteBad, Number, Quote, QuoteGap, QuoteBad, QuoteEnd,
    QuoteEndBad, Type, Key, Enum, Struct, Reserved, Function, Id, PreOp, InOp,
    PostOp, PreInOp, PrePostOp, Sign, PreSign, InSign, PostSign, Bad, ROpen,
    RClose, SOpen, SClose, COpen, CClose, BOpen, BClose, DOpen, DClose, Newline,
    Property,
};
enum { nTags = Property+1 };

// note state, quote state, enum/struct, PreIn, PrePost, brackets.

// Don't need NoteGap. Recognise spaces directly? No. Can't have two NoteGaps in
// a row, so can determine state from two preceding tokens (Note+Gap). Maybe
// don't need NoteBad similarly. What about "// /*/*/*/*/*/*" ? Context back to
// start of line makes it OK. Transfer Note to styles.

// Quote similarly, but what about QuoteEnd ? Can transfer Char to styles. Maybe
// transfer QuoteEnd somehow to styles.

// Enum, struct are extra keys. Only need one. ONE BIT SO FAR (Key+Other).
// PreIn = In+Other, so one bit.  PrePost = Post+Other, so one bit.

// STOP: overrides change the picture completely. Do we want to transfer
// everything to style and have just one tag type? Add to it for every language?
// What about C's }; versus } ? What happens to curly brackets in other
// languages? Want to take the same approach for all curly languages.

// Maybe go back to scan-by-line. Context is whole line. Store explicit state
// info at the end of a line, ready for the next. E.g. ready for
// non-block-open-curly. Synch by line. Buffer has room for one token per
// character. Length is one byte (token longer than 255, then split it). Maybe
// store tokens by line in that style!

// The information for each tag. Commented tags are filled in automatically.
tagInfo tags[nTags] = {
    [Gap] =         { Out,     Nonfix,  GAP,        Linear, Flat },
    [Note] =        { InNote,  Nonfix,  COMMENTED,  Linear, Flat },
    [NoteGap] =     { InNote,  Nonfix,  GAP,        Linear, Flat },
    [NoteBad] =     { InNote,  Nonfix,  WARN,       Linear, Flat },
    [Number] =      { Out,     Nonfix,  VALUE,      Linear, Flat },
    [Quote] =       { InQuote, Nonfix,  QUOTED,     Linear, Flat },
    [QuoteGap] =    { InQuote, Nonfix,  GAP,        Linear, Flat },
    [QuoteBad] =    { InQuote, Nonfix,  WARN,       Linear, Flat },
    [QuoteEnd] =    { Out,     Nonfix,  QUOTED,     Linear, Flat },
    [QuoteEndBad] = { Out,     Nonfix,  WARN,       Linear, Flat },
    [Type] =        { Out,     Nonfix,  TYPE,       Linear, Flat },
    [Key] =         { Out,     Nonfix,  KEY,        Linear, Flat },
    [Enum] =        { Out,     Nonfix,  KEY,        Linear, Flat },
    [Struct] =      { Out,     Nonfix,  KEY,        Linear, Flat },
    [Reserved] =    { Out,     Nonfix,  RESERVED,   Linear, Flat },
    [Function] =    { Out,     Nonfix,  FUNCTION,   Linear, Flat },
    [Id] =          { Out,     Nonfix,  ID,         Linear, Flat },
    [PreOp] =       { Out,     Prefix,  OP,         Linear, Flat },
    [InOp] =        { Out,     Infix,   OP,         Linear, Flat },
    [PostOp] =      { Out,     Postfix, OP,         Linear, Flat },
    [Sign] =        { Out,     Nonfix,  SIGN,       Linear, Flat },
    [PreSign] =     { Out,     Prefix,  SIGN,       Linear, Flat },
    [InSign] =      { Out,     Infix,   SIGN,       Linear, Flat },
    [PostSign] =    { Out,     Postfix, SIGN,       Linear, Flat },
    [Bad] =         { Out,     Nonfix,  BAD,        Linear, Flat },
    [ROpen] =       { Out,     Prefix,  SIGN,       Open,   Round },
    [RClose] =      { Out,     Postfix, SIGN,       Close,  Round },
    [SOpen] =       { Out,     Prefix,  SIGN,       Open,   Square },
    [SClose] =      { Out,     Postfix, SIGN,       Close,  Square },
    [COpen] =       { Out,     Prefix,  SIGN,       Open,   Curly },
    [CClose] =      { Out,     Postfix, SIGN,       Close,  Curly },
    [BOpen] =       { Out,     Prefix,  SIGN,       Open,   Block },
    [BClose] =      { Out,     Postfix, SIGN,       Close,  Block },
    [DOpen] =       { Out,     Prefix,  SIGN,       Open,   Delimiter },
    [DClose] =      { Out,     Postfix, SIGN,       Close,  Delimiter },
    [Newline] =     { Out,     Infix,   NEWLINE,    Linear, Flat },
    [Property] =    { Out,     Nonfix,  PROPERTY,   Linear, Flat },
};

// TODO: Fill in the overridden tags.
static void fillTags() {
    for (int i = 0; i < Commented; i++) {
        tags[i] = (tagInfo) { Out, Nonfix, COMMENTED, Linear, Flat };
    }
}

// Find the next state, fixity, style, bracket, level from a tag.
static int state(int tag) { return tags[tag].state; }
static int fix(int tag) { return tags[tag].fixity; }
static int style(int tag) { return tags[tag].style; }
static int bracket(int tag) { return tags[tag].bracket; }
static int level(int tag) { return tags[tag].level; }

// Tags must leave room for two override bits.
void testTags() {
    assert(nTags <= 64);
}
