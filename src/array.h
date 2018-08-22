// The Snipe editor is free and open source, see licence.txt.

// Flexible arrays. A flexible array has information stored before the pointer
// so that it can be indexed as normal. Careful conventions are needed to
// cope with the fact that an array may move when it is increaed in size.
// Synonym type names are used, e.g. (chars *) rather than (char *) as a
// reminder. Whenever the array size is increased, the variable holding it
// needs to be updated. Flexible arrays are passed by reference to functions
// which might increase them, e.g.
//
//   void f(chars **pxs) {
//     chars *xs = *pxs;
//     ...
//     xs = increase(pxs...);
//     ...
//   }
//
// The generic functions provided are not type safe. However, the
// -fsanitize=address compiler flag is likely to catch most errors.

// Create a zero-length array with the given size of items. The capacity is
// always at least one more than the length, so that the length can exclude
// a terminator if desired. The terminator is preserved across resizing.
void *newArray(int itemSize);
void freeArray(void *xs);

// Find the length of an array.
int size(void *xs);

// Increase the size of an array, making room to insert n items at index i. A
// pointer to the array variable is passed in so that it can be updated if the
// array is moved. The new array is also returned for convenience.
void *increase(void *pxs, int i, int n);

// Decrease the size of an array by deleting n items at index i.
void *decrease(void *xs, int i, int n);
