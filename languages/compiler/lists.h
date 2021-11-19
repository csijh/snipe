// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdbool.h>

// A list of strings is implemented as a flexible array.
struct list;
typedef struct list list;

// Create a new empty list.
list *newList();

// Free a list, with or without freeing up its strings.
void freeList(list *l, bool contents);

// Find the length.
int size(list *l);

// Get the i'th string in the list.
string *get(list *l, int i);

// Set the i'th string in the list.
void set(list *l, int i, string *s);

// Add a string to a list, returning its index.
int add(list *l, string *s);

// Remove the last string in the list.
string *pop(list *l);

// Set the length to zero.
void clear(list *l);

// Find the index of a string in a list, or return -1.
int find(list *l, string *s);

// Find a string in a list, adding it if not already present.
int findOrAdd(list *l, string *s);

// Split a string into a list of lines.
list *splitLines(string *s);

// Split a line into the given list of tokens.
list *splitWords(string *s);

// Sort a list of strings.
void sort(list *l);
