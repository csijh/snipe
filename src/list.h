// The Snipe editor is free and open source, see licence.txt.

// A list is a generic variable length array. Specific list types are defined
// as synonyms, replacing newList() and A() by type-specific versions. A few
// list types are defined here, and more can be defined as required.
struct list;
typedef struct list list;

// Lists of integers.
typedef list ints;
ints *newInts();
int *I(ints *xs);

// Lists of chars.
typedef list chars;
chars *newChars();
char *C(chars *s);

// Lists of strings.
typedef list strings;
strings *newStrings();
char **S(strings *xs);

// Create a new list. As C has no compile-time generic parameters, a run-time
// type is passed in, e.g. 'I' for integers. The item size is passed in so that
// new list types can be defined as needed, including lists of structs.
list *newList(char type, int itemSize);

// Free up a list and its array. To freeze a list, extract the array and call
// free(xs) instead.
void freeList(list *xs);

// Provide direct access to the array. The array is only valid until the next
// resize. This function can be wrapped for each list type to allow indexing,
// e.g. int *I(xs) { return A('I', xs); } allows I(xs)[n]. The single capital
// letter is a reminder to avoid storing I(xs) in a variable wherever possible.
void *A(char type, list *xs);

// Find the length of a list.
int length(list *xs);

// Change the length of a list.
void resize(list *xs, int n);

// Add room for n items in the list at index i <= length.
void expand(list *xs, int i, int n);

// Delete n items from a list at index i.
void delete(list *xs, int i, int n);
