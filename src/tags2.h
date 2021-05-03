// Snipe language compiler. Free and open source. See licence.txt.

// For each byte of the original text, store an original and override tag.
struct tags;
typedef struct tags tags;

// An original tag and corresponding override tag for one byte of text.
struct pair { char tag, over; };
typedef struct pair pair;

// NONE indicates no overriding. It also provides a sentinel tag at either end.
enum { NONE = '-' };

// Create a new tags object, given the sequence of tags relevant to a language.
// There should be at most 32 tags in the sequence (out of the possible 58) and
// tags used as overrides should be among the first 8.
tags *newTags(char const *tagSequence);

// Get the pair of tags at position i, or NONE if i is out of range.
pair getTags(tags *ts, int i);

// Get the active tag at position i or NONE, taking the override into account.
char getTag(tags *ts, int i);

// Set the tag at position i, with no override.
void setTag(tags *ts, int i, char t);

// Set or reset the override tag at position i.
void override(tags *ts, int i, char o);
