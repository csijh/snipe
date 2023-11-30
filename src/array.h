// Snipe editor. Free and open source, see licence.txt.

// TODO: could include gap buffers!
// An array is dynamic, used for direct access to raw data. It has a prefixed
// header containing the length, capacity, and unit size. It may move when
// reallocated. The adjust() function must be called with a = adjust(a,d) in
// case a moves. If any function f(a) calls adjust(a,d), it must be called with
// a = f(a) in case a moves. Alternatively, an array can be passed to a
// function in a field of a wrapper object.

// Allocate an initially empty array of elements of the given unit size.
void *newArray(int unit);

int length(void *a);

// Change length by d, which can be negative.
void *adjust(void *a, int d);

// Pre-allocate more memory, so that future adjust calls don't move the array.
void *ensure(void *a, int d);

// Report an error in printf style, and exit.
void error(char *format, ...);
