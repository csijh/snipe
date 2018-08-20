// The Snipe editor is free and open source, see licence.txt.

// Provide bracket matching and auto-indenting. Bracket matching can be applied
// to any programming language. It just changes the styles and thus the syntax
// highlighting. Indenting can be applied to any curly-bracket language. It is
// applied to a line after editing and before display to establish the right
// indent amount automatically.

// Only brackets marked by the scanner as signs are recognized. Others are
// assumed to be inside comments or strings. Mismatched brackets are
// marked as errors. Unmatched brackets increase or decrease the indent.
// Only one of a mismatched pair of brackets (] is marked as an error where
// reasonably possible, e.g. in cases (]..) and [..(] only the middle bracket
// is marked as an error. That means indenting doesn't change when editing
// inner brackets.

// Match brackets and calculate indent. The running indent from the previous
// line is passed in and updated. The indent for the current line is returned.
// The return value is not necessarily the same as the running indent, because
// it may be temporarily zero for a blank line, or temporarily half an indent
// less for a label.
int findIndent(int *runningIndent, int n, char const line[n], char styles[n]);
