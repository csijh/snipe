// The Snipe editor is free and open source, see licence.txt.

// Flexible arrays. A flexible array has information stored before the pointer
// so that it can be indexed as normal. Careful conventions are needed to
// cope with the fact that an array may move when it is resized. Synonym type
// names are used, e.g. (chars *) rather than (char *) as a reminder. Whenever
// the array is resized, the variable holding it needs to be updated, e.g.
// xs = reSize(xs...) rather than just reSize(xs...). Flexible arrays are
// passed by reference to functions which might update them, so that the
// caller's variable can be updated as well as the local variable e.g.
//
//   void f(chars **pxs) {
//     chars *xs = *pxs;
//     ...
//     xs = *pxs = reSize(xs...);
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

// Change the size of an array by n at index i, deleting (n < 0) or making
// room for an insertion (n > 0). Return the possibly moved array so that
// variables referring to it can be updated.
void *reSize(void *xs, int i, int n);
