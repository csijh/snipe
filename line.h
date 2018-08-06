// The Snipe editor is free and open source, see licence.txt.
#include "list.h"

// A list of 'lines' contains the positions just after each newline in the text.
// The length n of the list excludes the empty last line which holds the cursor
// when it is after the final newline, but the functions work on 0 <= row <= n.

// Find the start position of a line.
int startLine(ints *lines, int row);

// Find the end position of a line, after the newline.
int endLine(ints *lines, int row);

// Find the length of a given line, including the newline.
int lengthLine(ints *lines, int row);

// Find the row number of the line containing a position.
int findRow(ints *lines, int p);
