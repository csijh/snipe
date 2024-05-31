// The Snipe editor is free and open source. See licence.txt.
#include "style.h"

// A text object holds the contents of a text file in memory, together with the
// style and bracket matching information for displaying it. All access is
// (row,col) based. A row is a zero-based line number which may go beyond the
// physical end of the text. A col is a byte-index (not a "character" index)
// into the line which may go beyond the physical end of the line.
typedef struct text Text;

// Create or free a text object.
Text *newText();
void freeText(Text *t);

// Load a file, deleting any previous content.
void load(Text *t, char *path);

// The number of physical lines of text. The current row can be beyond the last.
int rows(Text *t);

// Make row r current.
void fixRow(Text *t, int r);

// Get the length of the given line (possibly beyond the physical end).
int lineLength(Text *t, int row);

// Get read only access to the given line.
char const *getLine(Text *t, int row);

// TODO: prepare for scanning.
//byte *getStyles ???

// TODO: does the bracket match the top opener?
bool matches(Text *t, byte bracket);

// TODO: push bracket.
void push(Text *t, int col, byte bracket);

// TODO: match or mismatch the bracket with the top opener.
void match(Text *t, int col, byte bracket);

// =====================================
// TODO: make the access line-based, with a current (caret) row.

// A line structure makes available the whole text, the whole styles array, the
// row number, and the from and to indexes of the line. The content is only
// valid until the next time the text is updated.
struct line { char *text; byte *styles; int row, from, to; };

// TODO: no T
// The total number of bytes of text (or styles).
int lengthT(Text *t);

// TODO: combined get line (for read or write)
// Get or set the i'th byte of text or the i'th style.
char getT(Text *t, int i);
byte getK(Text *t, int i);
void setT(Text *t, int i, char c);
void setK(Text *t, int i, byte k);

// TODO: no T
// Insert n text bytes from array a at index i. Add style bytes.
void insertT(Text *t, int i, char *a, int n);

// TODO: no T
// Delete n text bytes from index i, copying them into array a.
void deleteT(Text *t, int i, char *a, int n);

// TODO: no need?
// Copy n text bytes or n style bytes from index i into array a.
void copyT(Text *t, int i, char *a, int n);
void copyK(Text *t, int i, byte *a, int n);

// TODO: no T
// Get the cursor position.
int cursorT(Text *t);

// TODO: no T
// Move the cursor to the given position. (Has no effect other than to amortize
// long movements.)
void moveT(Text *t, int cursor);
