// The Snipe editor is free and open source. See licence.txt.

// File and directory handling. Find files relative to the installation
// directory or current directory, and read or write files or directories. In
// paths / is used exclusively as the separator, even on Windows. File names
// containing \ or / are ignored and directory names have / at the end.
#include <stdbool.h>

// Find the current working directory. Free the result with freeArray.
char *findCurrent();

// Find the installation directory using the args[0] from main() and the current
// working directory. Free the result with freeArray.
char *findInstall(char const *arg0, char const *current);

// Make a path from a format and some pieces. The first piece must be absolute.
// The format is used like sprintf, but producing an array. Free with freeArray.
char *makePath(char const *format, ...);

// Find the parent directory of the given path. Free with freeArray.
char *parentPath(char const *path);

// Find the extension of a filename or file path. The result is not newly
// allocated. It is a substring of the argument, or ".txt" if not recognized,
// or ".directory", or ".makefile".
char *extension(char const *path);

// Read in the contents of a text file into the given array, returning the
// possibly reallocated array. The path must not end in a slash. A final
// newline is added, if necessary, but not a null terminator. On failure, a
// message is printed and NULL is returned.
char *readFile(char const *path, char *content);

// Read in the contents of a directory into the given array, returning the
// possibly reallocated array. The path must end with a slash. The result has
// one line per name including ../ in natural order, with slashes on the end of
// subdirectory names. On failure, a message is printed and NULL is returned.
char *readDirectory(char const *path, char *content);

// Write the given data to the given file. On failure, a message is printed.
// For a makefile, indents are converted to tabs.
void writeFile(char const *path, int size, char data[size]);
