// A string is a flexible array of characters. Using the string typedef, the
// structure is read-only, but the characters are writable. A null character is
// maintained at the end, but a string may also contain null characters.

struct string { int max, length; char *s; };
typedef const struct string string;

// Create a new empty string or free it.
string *newString();
void freeString(string *s);

// A synonym for s->length.
int lengthString(string *s);

// Change the length of the string, adding a null.
void setLengthString(string *s, int n);

// Set the length to zero, ready to reuse. If big, reallocate as small.
void clearString(string *s);

// Copy a normal null terminated C string into a string.
void fillString(string *str, char *s);
