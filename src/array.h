// Snipe editor. Free and open source, see licence.txt.

// Provide support for dynamic arrays and gap buffers, accessed directly as
// normal C arrays. No distinction is made between the two, an array being a
// gap buffer with the gap at the end. Care needs to be taken when data might
// move, either because of reallocation, or because of a gap move. To avoid
// propagating these issues, either the array can be accessed via an owner
// object, or ensure() can be called to reallocate the array in advance of
// passing it to a function.

// Allocate an initially empty array of elements of the given unit size.
void *newArray(int unit);

void freeArray(void *a);

// The number of items in an array, also the start index of the gap in a buffer.
int length(void *a);

// The start of the high data in a buffer, also the end index of the gap.
int high(void *a);

// The capacity of an array, also the end of the high data in a buffer.
int max(void *a);

// Change length by d, which can be negative. For a gap buffer, this represents
// the insertion or deletion of data at the start of the gap.
void *adjust(void *a, int d);

// Change the high point by d, reallocating if necessary.
void *rehigh(void *a, int d);

// Pre-allocate more memory to allow for d more items, so that some future
// adjust calls don't move the array.
void *ensure(void *a, int d);

// Move the gap from length(a) to the given offset (without relocation).
void moveGap(void *a, int to);

// Report an error in printf style, and exit.
void error(char *format, ...);
