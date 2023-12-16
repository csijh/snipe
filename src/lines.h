// Snipe editor. Free and open source, see licence.txt.

// Track and access lines in the text.
typedef struct lines Lines;

// Create or free a lines object.
Lines *newLines();
void freeLines(Lines *ls);

// The total number of lines.
int sizeL(Lines *ls);

// Get the start or end position of the i'th line. If i >= sizeL, return the end
// of the text.
int startL(Lines *ls, int i);
int endL(Lines *ls, int i);

// Respond to an insertion of n bytes at index p in the text, by adjusting the
// line boundaries after the insertion point. Also add lines corresponding to
// any newlines in the insertion.
void insertL(Text *t, int i, char *a, int n);

// Respond to a deletion of n bytes from the text at index p, by adjusting the
// line boundaries after the insertion point. Also remove lines corresponding
// to any newlines in the deletion.
void deleteL(Text *t, int i, char *a, int n);
