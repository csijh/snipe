// The Snipe editor is free and open source, see licence.txt.
#include "list.h"
#include <stdbool.h>

// A generic scanner for syntax-based highlighting or editing. It is implemented
// as a string-matching state machine, with the details supplied from a language
// definition file such as languages/c.txt
struct scanner;
typedef struct scanner scanner;

// Create a scanner for plain text.
scanner *newScanner(void);

// Free a scanner and its internal data.
void freeScanner(scanner *sc);

// Change the language of the scanner. The language should be a file extension
// such as "c" or "java". The default is plain text.
void changeLanguage(scanner *sc, char *lang);

// Scan a line of text. All previous lines of the same document must already
// have been scanned. The styles array is filled with the resulting bytes,
// ending with a null byte to match the line's null character.
void scan(scanner *sc, int row, chars *line, chars *styles);
