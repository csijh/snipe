// Snipe editor. Free and open source, see licence.txt.
#include <stdbool.h>
#include <stdio.h>

// A text object holds the content of a file. The text is kept clean so that it
// can be saved at any moment by autosave. That means the text is UTF8-valid,
// has no control characters other than newlines, has no trailing spaces at the
// ends of lines, has no blank lines at the end, and has a final newline.
struct text;
typedef struct text text;

// A point in the text is a row, i.e. a zero-based line number, and a col, i.e.
// a zero-based byte index within a line. A line is a sequence of bytes ending
// with a newline '\n'.
struct point { int row, col; };
typedef struct point point;

// Create an empty text object.
text *newText();

// Free a text object and its data.
void freeText(text *t);

// Find the number of rows (i.e. number of newlines) in the text.
int rows(text *t);

// TODO: length of line (inc?)

// Get a read-only and volatile pointer to a given line of text, terminated by
// newline. If row >= rows(t), a blank line is returned.
char const *line(text *t, int row);

// Insert n bytes at a given row/col point, returning false if the bytes are
// not UTF8-valid. The col can be beyond the end of a line, and the row can be
// beyond the end of the file, with spaces or newlines being added accordingly.
// After the operation, the text is cleaned up again.
bool insert(text *t, point p, int n, char s[n]);

// Delete bytes from point p to point q. Either column can be beyond the end of
// a line, and either row can be beyond the end of the file, in which case
// notional spaces or newlines are deleted. The actual deleted bytes are copied
// into the given character array, if it is not null, followed by a null
// character. If the size n of the array is insufficient, false is returned, and
// the deletion is not performed. The caller tries again with n increased. After
// the operation, the text is cleaned up again.
bool delete(text *t, point p, point q, int n, char s[n]);

// Fill a text object from an open file, discarding any previous content. The
// file should have been opened in binary mode, so that the file length is the
// same as the number of bytes read in. Return false if the buffer contains
// invalid UTF-8 sequences, and otherwise clean the text and return true.
bool loadText(text *t, int n, FILE *f);

// Copy the text out into an open file. The file should have been opened in
// binary mode, so that line endings are preserved.
char *saveText(text *t, FILE *f);
