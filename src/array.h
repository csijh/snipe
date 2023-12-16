// Snipe editor. Free and open source, see licence.txt.
#include <limits.h>

// Provide support for dynamic arrays, accessed directly as normal C arrays.
// These are mostly used for temporary storage of data while it is being moved
// around. Care needs to be taken because an array might move because of
// reallocation. To avoid problems, call ensure() to reallocate the array in
// advance of passing it to a function.

// Create an initially empty array of items of the given unit size. The elements
// are assumed not to need more than pointer alignment. Free an array.
void *newArray(int unit);
void freeArray(void *a);

// The number of items in an array.
int length(void *a);

// Change length by d, which can be negative.
void *adjust(void *a, int d);

// Pre-allocate to allow for d more items, so that future adjust calls up to
// that amount don't move the array.
void *ensure(void *a, int d);

// Report an error in printf style, and exit.
void error(char *format, ...);
