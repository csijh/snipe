// Snipe editor. Free and open source, see licence.txt.

// Store a file in memory for editing.

// TODO: use 4 gap buffers.
// Store an array of variable-length strings, e.g. the lines or tags of a file.
struct document;
typedef struct document Document;

// Create an empty document.
Document *newFile();

// Replace the File by the given text, e.g. the contents of a newly loaded
// document, as one array. The text is replaced by the old contents of the File.
// TODO: copy for safety? Or load = readFile!
void replace(File *s, text *t);

// Write to document.
void save(File *s, char *documentname);

// Copy a row into the given text object. If past the end of the rows, return
// an empty text.
void get(File *s, int row, text *t);

// Copy the text into a given row. If past the end and not empty, extend.
void put(File *s, int row, text *t);

// Split a line.
void split(File *s, int row, int col);

// Join a line with the next, if any.
void join(File *s, int row);
