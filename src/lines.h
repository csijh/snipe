// The Snipe editor is free and open source. See licence.txt.

// Track lines in the text, indexed by row, i.e. zero-based line number.
typedef struct lines Lines;

// Create or free a lines object.
Lines *newLines();
void freeLines(Lines *ls);

// The total number of physical lines, equal to the number of newlines.
int sizeL(Lines *ls);

// Get the start position of a line. If row >= sizeL, return the position just
// before the final newline, i.e. provide a virtual blank line.
int startL(Lines *ls, int row);

// Get the end position of a line, after the newline. If row >= sizeL,
// return the position after the final newline.
int endL(Lines *ls, int row);

// Find the length of a line, including the newline.
int lengthL(Lines *ls, int row);

// Respond to an insertion of n bytes at index p in the text, by adjusting the
// line boundaries after the insertion point. Also add lines corresponding to
// any newlines in the inserted text.
void insertL(Lines *ls, int p, char *s, int n);

// Respond to a deletion of n bytes from the text at index p, by adjusting the
// line boundaries after the deletion point. Also remove lines corresponding to
// any newlines in the deleted text.
void deleteL(Lines *ls, int p, char *s, int n);
