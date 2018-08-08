// The Snipe editor is free and open source, see licence.txt.
#include <stdbool.h>

// A list is a variable length array of chars, ints, or strings. Some of the
// functions are made generic with the _Generic feature of C11.
struct chars;
typedef struct chars chars;
struct ints;
typedef struct ints ints;
struct strings;
typedef struct strings strings;

// Create an empty list.
chars *newChars(void);
ints *newInts(void);
strings *newStrings(void);

// Free a list.
void freeChars(chars *list);
void freeInts(ints *list);
void freeStrings(strings *list);

// Print a character list.
void print(chars *cs);

// Find the current length of a list.
#define length(list) _Generic((list), \
    chars *: lengthC, \
    ints *: lengthI, \
    strings *: lengthS)(list)
int lengthC(chars *list);
int lengthI(ints *list);
int lengthS(strings *list);

// Get an item from a list, equivalent to xs[i].
#define get(list, i) _Generic((list), \
    chars *: getC, \
    ints *: getI, \
    strings *: getS)(list, i)
char getC(chars *list, int i);
int getI(ints *list, int i);
char *getS(strings *list, int i);

// Set an item in a list, equivalent to xs[i] = x.
#define set(list, i, x) _Generic((list), \
    chars *: setC, \
    ints *: setI, \
    strings *: setS)(list, i, x)
void setC(chars *list, int i, char b);
void setI(ints *list, int i, int n);
void setS(strings *list, int i, char *s);

// Add an item to a list.
#define add(list, x) _Generic((list), \
    chars *: addC, \
    ints *: addI, \
    strings *: addS)(list, x)
void addC(chars *list, char c);
void addI(ints *list, int n);
void addS(strings *list, char *s);

// Add another list onto the end of a list.
#define addList(list, xs) _Generic((list), \
    chars *: addListC, \
    ints *: addListI, \
    strings *: addListS)(list, xs)
void addListC(chars *list, chars *cs);
void addListI(ints *list, ints *ns);
void addListS(strings *list, strings *ss);

// Insert items into a list from an array.
#define insert(list, i, n, a) _Generic((list), \
    chars *: insertC, \
    ints *: insertI, \
    strings *: insertS)(list, i, n, a)
void insertC(chars *list, int i, int n, char *a);
void insertI(ints *list, int i, int n, int *a);
void insertS(strings *list, int i, int n, char **a);

// Delete n items starting at index i.
#define delete(list, i, n) _Generic((list), \
    chars *: deleteC, \
    ints *: deleteI, \
    strings *: deleteS)(list, i, n)
void deleteC(chars *list, int i, int n);
void deleteI(ints *list, int i, int n);
void deleteS(strings *list, int i, int n);

// Copy a portion of the list into the given array.
#define copy(list, p, n, a) _Generic((list), \
    chars *: copyC, \
    ints *: copyI, \
    strings *: copyS)(list, p, n, a)
void copyC(chars *list, int p, int n, char *a);
void copyI(ints *list, int p, int n, int *a);
void copyS(strings *list, int p, int n, char **a);

// Copy part of a list into another list.
#define sublist(list, p, n, sub) _Generic((list), \
    chars *: sublistC, \
    ints *: sublistI, \
    strings *: sublistS)(list, p, n, sub)
void sublistC(chars *list, int p, int n, chars *sub);
void sublistI(ints *list, int p, int n, ints *sub);
void sublistS(strings *list, int p, int n, strings *sub);

// Compare a sublist with an array.
#define match(list, p, n, a) _Generic((list), \
    chars *: matchC, \
    ints *: matchI, \
    strings *: matchS)(list, p, n, a)
bool matchC(chars *list, int p, int n, char *a);
bool matchI(ints *list, int p, int n, int *a);
bool matchS(strings *list, int p, int n, char **a);

// Change the length of a list.
#define resize(list, n) _Generic((list), \
    chars *: resizeC, \
    ints *: resizeI, \
    strings *: resizeS)(list, n)
void resizeC(chars *list, int length);
void resizeI(ints *list, int length);
void resizeS(strings *list, int length);

// Convert a list to a fixed size array, and free the list structure.
#define freeze(list) _Generic((list), \
    chars *: freezeC, \
    ints *: freezeI, \
    strings *: freezeS)(list)
char *freezeC(chars *list);
int *freezeI(ints *list);
char **freezeS(strings *list);
