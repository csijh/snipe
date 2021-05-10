// Snipe language compiler. Free and open source. See licence.txt.
#include <stdbool.h>

// A tag has a name which starts with an upper case letter or consists of a
// single ASCII symbol. Only the first character of the name is significant, but
// consistency is checked, i.e. there must not be two names with the same first
// character. A tag is classified as a bracket or a delimiter or neither, and as
// an opener or closer or neither or both.
struct tag;
typedef struct tag tag;

// List of tags.
struct tags;
typedef struct tags tags;

// The MORE tag is the default for a missing tag in a language description,
// indicates a continuation character of a token in the scanner, and specifies
// no effect on the tags between brackets in the matcher. The SKIP tag labels a
// lookahead rule in a language description, tags a continuation byte of a UTF-8
// character or grapheme or flags a state transition table entry which isn't
// relevant in the scanner, or acts as a sentinel representing the start or end
// of the entire text in the matcher. The GAP tag is the tag for a space
// character when it is between tokens. The NEWLINE tag is the tag for a newline
// when it is between tokens.
enum { MORE = '-', SKIP = '~', GAP = '_', NEWLINE = '.' };

// Create the list of tags.
tags *newTags();

// Free the tags list.
void freeTags(tags *ts);

// Find the tag with given character.
tag *findTag(tags *ts, int ch);

// Get a tag's character.
char tagChar(tag *t);

// Check whether a character is an ASCII symbol.
bool isSymbol(char ch);

// Set the role of a tag. A tag can't be both a bracket and a delimiter.
void setBracket(tag *t, int row);
void setDelimiter(tag *t, int row);
void setOpener(tag *t, int row);
void setCloser(tag *t, int row);

// Check if a tag is a bracket or delimiter or opener or closer.
bool isBracket(tag *t);
bool isDelimiter(tag *t);
bool isOpener(tag *t);
bool isCloser(tag *t);
