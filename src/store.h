// TODO: turn into document or file!
// TODO: use 4 gap buffers.
// Store an array of variable-length strings, e.g. the lines or tags of a file.
struct store;
typedef struct store store;

// Create an empty store.
store *newStore();

// Replace the store by the given text, e.g. the contents of a newly loaded
// file, as one array. The text is replaced by the old contents of the store.
// TODO: copy for safety? Or load = readFile!
void replace(store *s, text *t);

// Write to file.
void save(store *s, char *filename);

// Copy a row into the given text object. If past the end of the rows, return
// an empty text.
void get(store *s, int row, text *t);

// Copy the text into a given row. If past the end and not empty, extend.
void put(store *s, int row, text *t);

// Split a line.
void split(store *s, int row, int col);

// Join a line with the next, if any.
void join(store *s, int row);
