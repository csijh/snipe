// Snipe file and directory handling. Free and open source. See licence.txt.

// Find the installation directory, current directory on startup, and the user's
// home and preference directories. Normalize and build paths. Read and write
// files. Read directories. In paths / is used exclusively as the separator.
// (Windows libraries accept / even though Windows apps don't.) File names
// containing \ or / are ignored and directory names have / at the end.
#include "unicode.h"

// A files object holds system paths and supports read/write of files/dirs.
struct files;
typedef struct files files;

// Create a files object, passing in args[0] from the call to main, to allow the
// installation directory to be found.
files *newFiles(char const *arg0);

// Release a files object.
void freeFiles(files *fs);

// Get one of the system directories. The user preferences directory is
// $HOME/.config/ on Linux, $HOME/Library/Preferences/ on macOS,
// $HOME/AppData/Roaming/ on Windows.
char *installDir(files *fs);
char *currentDir(files *fs);
char *homeDir(files *fs);
char *prefsDir(files *fs);

// Expand and normalize a file name or path to a full path, relative to the
// current directory if not already absolute. Convert \ to / and add a trailing
// slash for a directory. Free the result string as soon as possible.
char *fullPath(files *fs, char const *file);

// Find the parent directory of the given path. Free the result ASAP.
char *parentPath(char const *path);

// Build a string from n parts. Free the result as soon as possible.
char *join(int n, ...);

// Find the extension of a filename or file path, without the dot. The result is
// not newly allocated; it is a substring of the argument. If there is no
// extension, the result is "directory" or "makefile" or "txt".
char *extension(char const *path);

// Check that a file exists, and return its size or -1.
int fileSize(char const *path);

// Read in the contents of a text file or directory. For a file, a final newline
// is added, if necessary, plus a null terminator. For a directory, there is one
// line per name in natural order, including the full path of the directory and
// ../ at the start, with slashes on the end of the directory names. On failure,
// a message is printed and NULL is returned.
char *readPath(char const *path);

// Write the given data to the given file. On failure, a message is printed.
void writeFile(char const *path, int size, char data[size]);
