// The Snipe editor is free and open source, see licence.txt.
#include "array.h"

// Synonym for flexible array of characters with terminator.
typedef char string;

// Provide bracket matching and auto-indenting. Bracket matching can be applied
// to any programming language. It just changes the styles and thus the syntax
// highligting. Indenting can be applied to any curly-bracket language. It is
// applied to a line after editing and before display to establish the right
// indent amount automatically.

// Match brackets. Only brackets marked SIGN are recognized, others are assumed
// to be inside comments or strings. After matching, matched brackets remain
// marked SIGN, mismatched brackets are marked BAD, and unmatched brackets are
// marked OPEN or CLOSE. Only one of a mismatched pair of brackets is marked as
// BAD where reasonably possible.
void matchBrackets(string *line, string *styles);

// Correct the indenting of a line, given the running indent and the result of
// bracket-matching. Return the new running indent.
int autoIndent(int indent, string **linep, string **stylesp);
