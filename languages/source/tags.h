// Snipe language compiler. Free and open source. See licence.txt.
#include <stdbool.h>

// A tag is either a name starting with an upper case letter or consists of a
// single ASCII symbol. Only the first letter of a name is significant, but
// consistency is checked, i.e. names of more than one character must have the
// same first letter. A tag is classified as a bracket or a delimiter or
// special, and as an opener or closer or both. Read-only access to the
// structure is provided via the tag type.
struct tag {
    bool bracket, delimiter, opener, closer;
    char ch;
    char name1[2];
    char name[];
};
typedef struct tag const tag;

// Find an existing tag or create a newly allocated one.
tag *findTag(tag ***tsp, char *s);

// Deallocate a tag.
void freeTag(tag *t);

// Check whether a string is a single ASCII symbol.
bool isSymbol(char *s);

// Reserved tags: 'more' indicates a continuation character of a token, or the
// start or end of the entire text, or indicates no overriding; 'skip' indicates
// a continuation byte of a UTF-8 character or grapheme, or a state transition
// table entry which isn't relevant, or a lookahead rule; 'gap' is the tag for a
// space when it is between tokens; 'newline' is the tag for a newline when it
// is between tokens.
extern tag *more, *skip, *gap, *newline;
