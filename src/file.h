// The Snipe editor is free and open source. See licence.txt.

// File and directory handling. Find files relative to the installation
// directory or current directory, and read or write files or directories. In
// paths / is used exclusively as the separator, even on Windows. File names
// containing \ or / are ignored and directory names have / at the end.
#include <stdbool.h>

// Find the installation directory and current working directory from args[0].
void findResources(char const *arg0);

// Free up resource path strings when shutting down.
void freeResources(void);

// Get the full path of a resource, given its / terminated installation
// subdirectory, relative file name, and extension. The result string should
// be freed as soon as it is no longer needed.
char *resourcePath(char *directory, char *file, char *extension);

// Create an allocated path string from a slash terminated directory path and a
// file name. Deal with the file being . or .. or an absolute file path.
char *addPath(char const *path, char const *file);

// Create an allocate string containing the parent directory of the given path.
char *parentPath(char const *path);

// Expand a file name to a full path, relative to the current directory, if not
// already absolute. Convert \ to / and add a training slash for a directory.
// The result string should be freed as soon as it is no longer needed.
char *fullPath(char const *file);

// Find the extension of a filename or file path, without the dot. The result
// is not newly allocated. It is a substring of the argument, or "directory",
// or "makefile". It is "txt" if not recognized.
char *extension(char const *path);

bool secure(const char *path);

// Read in the contents of a text file into the given array, returning the
// possibly reallocated array. The path must not end in a slash. A final
// newline is added, if necessary, but not a null terminator. On failure, a
// message is printed and NULL is returned.
char *readFile(char const *path, char *content);

// Read in the contents of a directory. The path must end with a slash. The
// result has one line per name including ../ in natural order, with slashes on
// the end of subdirectory names. On failure, a message is printed and NULL is
// returned.
char *readDirectory(char const *path);

// Write the given data to the given file. On failure, a message is printed.
void writeFile(char const *path, int size, char data[size]);
