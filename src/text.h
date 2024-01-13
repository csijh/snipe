// The Snipe editor is free and open source. See licence.txt.
#include "kinds.h"

// Store the source text, with the bytes accessed using array-like indexes. For
// each byte of text, there is also a byte representing its kind (start of
// token of particular type, or continuation byte).
typedef struct text Text;

// Create or free a text object.
Text *newText();
void freeText(Text *t);

// The total number of bytes of text (or kinds).
int lengthT(Text *t);

// Get or set the i'th byte of text or the i'th kind.
char getT(Text *t, int i);
Kind getK(Text *t, int i);
void setT(Text *t, int i, char c);
void setK(Text *t, int i, Kind k);

// Insert n text bytes from array a at index i. Add kind bytes (set to None).
void insertT(Text *t, int i, char *a, int n);

// Delete n text bytes from index i, copying them into array a.
void deleteT(Text *t, int i, char *a, int n);

// Copy n text bytes or n kind bytes from index i into array a.
void copyT(Text *t, int i, char *a, int n);
void copyK(Text *t, int i, Kind *a, int n);

// Get the cursor position, or move the cursor to the given position. (Has no
// effect other than to amortize long movements.)
int cursorT(Text *t);
void moveT(Text *t, int cursor);
