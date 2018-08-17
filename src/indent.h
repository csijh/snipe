// The Snipe editor is free and open source, see licence.txt.
#include "array.h"

// Synonym for flexible array of characters with nul terminator.
typedef char string;

// Provide bracket matching and auto-indenting. Bracket matching can be applied
// to any programming language. It just changes the styles and thus the syntax
// highligting. Indenting can be applied to any curly-bracket language. It is
// applied to a line after editing and before display to establish the right
// indent amount automatically.

// Match brackets. Only brackets marked by the scanner as signs are recognized.
// Others are assumed to be inside comments or strings. Mismatched brackets are
// marked as errors. Unmatched brackets are temporarily flagged as indenters or
// outdenters, ready for auto-indenting. Only one of a mismatched pair of
// brackets (] is marked as an error where reasonably possible, e.g. in cases
// (]..) and [..(] only the middle bracket is marked as an error.
void matchBrackets(string *line, string *styles);

// Correct the indenting of a line, given the running indent and the result of
// bracket-matching. Return the new running indent.
int autoIndent(int indent, string **linep, string **stylesp);
