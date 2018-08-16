// The Snipe editor is free and open source, see licence.txt.

// Flexible arrays. A flexible array has information stored before the pointer
// so that it can be indexed as normal. Careful conventions are needed to
// cope with the fact that an array may move when it is resized. Synonym type
// names are used, e.g. (chars *) rather than (char *) as a reminder. Whenever
// the array is resized, the variable holding it needs to be updated, e.g.
// xs = setSize(xs...) rather than just setSize(xs...). Flexible arrays are
// passed by reference to functions which might update them, so that the
// caller's variable can be updated as well as the local variable e.g.
//
//   void f(chars **pxs) {
//     chars *xs = *pxs;
//     ...
//     xs = *pxs = setSize(xs...);
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

// Change the size of an array to n, and return the possibly moved array so
// that variables referring to it can be updated. If the size is increased,
// room is made for more items at position i where 0 <= i <= size(xs). If the
// size is decreased, items are deleted from position i.
void *setSize(void *xs, int i, int n);
