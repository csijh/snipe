// Snipe language compiler. Free and open source. See licence.txt.
#include <stdbool.h>

// A tags structure stores an array of original and override tag pairs.
struct tags;
typedef struct tags tags;

// An original tag and corresponding override tag.
struct pair { char tag, over; };
typedef struct pair pair;

// NONE indicates no overriding. It also provides a sentinel tag at either end.
enum { NONE = '-' };

// Create a tags object, given the tags relevant to a language. There should be
// at most 32 tags (out of the possible 58) of which those used as overrides
// should appear among the first 8.
tags *newTags(char *tagChars);

// Fill in the tags from s (with no overriding).
void fillTags(tags *ts, char *s);

// Find the length of the tags array.
int countTags(tags *ts);

// Get the pair of tags at the given position.
pair getTags(tags *ts, int i);

// Get the active tag at the given position, taking the override into account.
char getTag(tags *ts, int i);

// Set or reset the override tag at the given position.
void override(tags *ts, int i, char o);
