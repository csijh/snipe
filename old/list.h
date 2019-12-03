// The Snipe editor is free and open source, see licence.txt.

// A list is a variable length array. Generic functions are provided, and some
// specific list types are defined as synonyms, with their own type-specific
// functions. Further specific types can be defined as required. A minor form
// of dynamic type checking counteracts the lack of compile time type security.
// The capacity of a list is always one more than its length, to allow for a
// terminator or other extra information at the end, and that final element is
// preserved across resizes.
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

// Find a pointer to the i'th element of a list. This can be used for
// lists of structs, for example.
void *getp(list *xs, int i);

// Provide direct access to the array, which is only valid until the next
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
