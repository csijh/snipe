// The Snipe editor is free and open source. See licence.txt.
#include <limits.h>
#include <stdbool.h>

// Dynamic arrays, accessed directly as normal C arrays. Care needs to be taken
// because an array might move because of reallocation. To pass an array to a
// function: (a) make sure the function doesn't reallocate it or (b) call
// padTo/padBy in advance or (c) return the array as the result or (d) pass the
// array as a field of an owning object. The array module also provides error
// and warning functions.

// The string type indicates that the string is held in an array. The length()
// call excludes the null terminator, as with strlen.
typedef char *string;

// Create an initially empty array of items of the given unit size. The elements
// are assumed not to need more than pointer alignment. Free an array.
void *newArray(int unit);
void freeArray(void *a);

// The number of items in an array.
int length(void *a);

// Change length to n or by d, returning the possibly reallocated array.
void *adjustTo(void *a, int n);
void *adjustBy(void *a, int d);

// Pre-allocate to n items or by d items without changing the length.
void *padTo(void *a, int n);
void *padBy(void *a, int d);

// Report an error in printf style, adding a newline, and exit.
void error(char const *format, ...);

// Check a boolean. Print an error and exit if false.
void check(bool ok, char const *format, ...);

// Print a warning and return NULL.
void *warn(char const *format, ...);
