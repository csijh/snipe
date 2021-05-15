// Snipe text handling. Free and open source, see licence.txt.
// TODO: get() with +- arg, shared with tag bytes.
#include <stdbool.h>

// A text object holds the UTF-8 content of a file. The text is kept clean so
// that it can be saved at any moment by autosave. That means the text never
// contains invalid UTF-8 sequences, or carriage returns. In addition, edits are
// adjusted to avoid trailing spaces at the ends of lines, trailing blank lines,
// or a missing final newline. The bytes are held in a gap buffer. For n bytes,
// there are n+1 positions in the text, running from 0 (at the start) to n
// (after the final newline).
struct text;
typedef struct text text;

// Provide edit opcodes for insertion, deletion and copying of text.
enum { INSERT = 0, DELETE = 1, COPY = -1 };

// Store an edit (insertion, deletion, copying or other operation such as a
// cursor change). The op is one of the above, or another opcode defined
// elsewhere. The at and to fields are the start and end positions in the text
// (0 <= at <= to <= length of text), and s holds n bytes (n <= max).
struct edit { int op, at, to, n, max; char s[]; };
typedef struct edit edit;

// Get an edit structure big enough to hold n characters and a null. Pass in an
// old edit structure to be reused, or NULL to get a fresh edit structure.
edit *newEdit(int n, edit *old);

// Create an empty text object.
text *newText();

// Free a text object and its data.
void freeText(text *t);

// Return the number of bytes. The maximum length is INT_MAX, so that int can be
// used for positions, and for positive or negative relative positions.
int lengthText(text *t);

// Fill a text object from a newly loaded file, discarding any previous content.
// Return false if the buffer contains invalid UTF-8 sequences, and otherwise
// clean the buffer and return true.
bool loadText(text *t, int n, char *buffer);

// Copy the text out into a buffer.
char *saveText(text *t, char *buffer);

// Do an insertion or deletion or copy, adjust the edit to keep the text clean,
// reallocate the edit structure if necessary, and return it. For an insertion,
// to=at is the insertion position. If at<to after adjustment, that indicates a
// range of spaces to be deleted and replaced by the insertion. For a deletion,
// at<to represents the range to be deleted. On return, the text that was
// deleted is filled in. A copy is like a deletion, but with no actual change.
edit *editText(text *t, edit *e);
