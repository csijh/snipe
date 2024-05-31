// The Snipe editor is free and open source. See licence.txt.
#include <limits.h>
#include <stdbool.h>

// Dynamic arrays, accessed directly as normal C arrays. Care needs to be taken
// because an array might move because of reallocation. To pass an array to a
// function: (a) make sure the function doesn't reallocate it or (b) call
// ensure in advance or (c) return the array as the result or (d) pass the
// array as a field of an owning object. The array module also provides gap
// buffer functions, and error and warning functions.

// Create or free an initially empty array of items of the given unit size.
void *newArray(int unit);
void freeArray(void *a);

// The number of items in an array.
int length(void *a);

// Change an array length to n, returning the possibly reallocated array.
void *resize(void *a, int n);

// Change an array length by d, returning the possibly reallocated array.
void *adjust(void *a, int d);

// Remove everything from an array.
void clear(void *a);

// Make sure the array has enough capacity for m more items. Return the possibly
// reallocated array.
void *ensure(void *a, int m);

// When an array is used as a gap buffer, it has extra entries between high(a)
// which is after the gap, and max(a) which is the capacity. Gap buffers are
// difficult to beat for simplicity and efficiency. See:
//     https://coredumped.dev/2023/08/09/text-showdown-gap-buffers-vs-ropes/
int high(void *a);
int max(void *a);

// Change the high point to n, to add or delete entries after the gap. Return
// the possibly reallocated array.
void *setHigh(void *a, int n);

// Move the gap from length(a) to n.
void moveGap(void *a, int n);

// Report an error in printf style, adding a newline, and exit.
void error(char const *format, ...);

// Check a boolean. Print an error and exit if false.
void check(bool ok, char const *format, ...);

// Print a warning and return NULL.
void *warn(char const *format, ...);
