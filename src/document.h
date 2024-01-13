// The Snipe editor is free and open source. See licence.txt.

// Store a file in memory for editing.

// TODO: use 4 gap buffers.
// Store an array of variable-length strings, e.g. the lines or tags of a file.
struct document;
typedef struct document Document;

// Create an empty document.
Document *newDocument();

// Replace the Document by the given text, e.g. the contents of a newly loaded
// document, as one array. The text is replaced by the old contents of the Document.
// TODO: copy for safety? Or load = readDocument!
void replace(Document *s, text *t);

// Write to document.
void save(Document *s, char *documentname);

// Copy a row into the given text object. If past the end of the rows, return
// an empty text.
void get(Document *s, int row, text *t);

// Copy the text into a given row. If past the end and not empty, extend.
void put(Document *s, int row, text *t);

// Split a line.
void split(Document *s, int row, int col);

// Join a line with the next, if any.
void join(Document *s, int row);

// ----------------
// Is it a simple array of line boundaries, or an array of line structures?
// Line structure contains:
//    line boundary (negative after gap)
//    scanner state (or store against newline)
//    bracket boundary (closers and openers for a line)
//    (#closers, #openers)
