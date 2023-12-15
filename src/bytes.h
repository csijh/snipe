// Snipe editor. Free and open source, see licence.txt.

// A gap buffer of bytes (e.g. for text, token types, or scan states). The
// buffer uses the char type, which could be signed or unsigned. The buffer can
// be indexed as an array, as if it didn't have a gap.
typedef struct bytes Bytes;

// Create or free a gap buffer.
Bytes *newBytes();
void freeBytes(Bytes *bs);

// The total number of bytes (either side of the gap).
int size(Bytes *bs);

// Get or set the i'th byte.
char get(Bytes *bs, int i);
void set(Bytes *bs, int i, char b);

// Insert or replace n bytes from s at index i.
void insert(Bytes *bs, int i, char *s, int n);
void replace(Bytes *bs, int i, char *s, int n);

// Copy or copy-and-delete n bytes from index i into s.
void copy(Bytes *bs, int i, char *s, int n);
void delete(Bytes *bs, int i, char *s, int n);

// Get the cursor position, or move the cursor to the given position. (Has no
// effect other than to amortize long movements.)
int cursor(Bytes *bs);
void move(Bytes *bs, int cursor);

// Load a file (deleting any previous content) or save the content into a file.
void load(Bytes *bs, char *path);
void save(Bytes *bs, char *path);
