// The Snipe editor is free and open source, see licence.txt.
#include "op.h"

// Store information about the lines in the text. There is one line for each
// newline, and "row" means "line number".
struct lines;
typedef struct lines lines;

// Create or free a lines object.
lines *newLines();
void freeLines(lines *ls);

// The number of lines in the text, equal to the number of newlines.
int countLines(lines *ls);

// Track insertions and deletions to keep line information up to date.
void changeLines(lines *ls, op *o);

// Find the start position of a line.
int startLine(lines *ls, int row);

// Find the end position of a line, after the newline.
int endLine(lines *ls, int row);

// Find the length of a given line, including the newline.
int lengthLine(lines *ls, int row);

// Find the row number of the line containing a position.
int findRow(lines *ls, int at);
