// Flexible arrays and strings. Free and open source. See licence.txt.

// A flexible array has information stored in a prefix so that it can be indexed
// as normal, for maximum convenience. Careful conventions are needed to cope
// with the fact that an array may move when it is increased in size. Mistakes
// are easily made, e.g. calling f(a,...) and not a = f(a,...). Use the compiler
// flag -fsanitize=address to catch errors.

// Create a new empty array containing items of the given size. The capacity is
// always at least one more than the length, so that the length can exclude a
// terminator, e.g. for null-terminated strings.
void *newArray(int stride);

// Free up an array. Free cannot be called directly because of the prefix.
void freeArray(void *a);

// Find the length of an array.
int lengthArray(void *a);

// Set the length of an array and return it. If the length (plus one) is greater
// than the capacity, a reallocated array is returned.
void *resizeArray(void *a, int n);

// Delete all the elements of an array, i.e. resize to zero.
void clearArray(void *a);

// A position can be associated with an array, e.g. if it represents an edit.
// Get or set the associated position.
int toArray(void *a);
void setToArray(void *a, int at);

// An opcode can be associated with an array, e.g. if it represents an edit.
// Get or set the associated opcode.
int opArray(void *a);
void setOpArray(void *a, int op);

// A string variable is a string stored as a character array. These are synonyms
// are provided for the array functions, except that s null character is
// maintained at the end. However, a string may also contain nulls.
typedef char string;
string *newString();
string *fillString(string *str, char *s);
void freeString(string *s);
int lengthString(string *s);
void *resizeString(string *s, int n);
void clearString(string *s);
int toString(string *s);
void setToString(string *s, int at);
int opString(string *s);
void setOpString(string *s, int op);
