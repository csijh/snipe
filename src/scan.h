// The Snipe editor is free and open source. See licence.txt.
#include "style.h"

typedef unsigned char byte;

// The state machine table has a row for each state, and an overflow area. Each
// row consists of 96 cells of 2 bytes each, for \n and \s and !..~. The
// scanner uses the current state and the next character in the source text to
// look up a cell. The cell may be an action, i.e. a style and a target state,
// for that single character, or an offset relative to the start of the table
// to a list of patterns in the overflow area starting with that character,
// with their actions.
enum { COLUMNS = 96, CELL = 2 };

// Flags added to the style in a cell. The LINK flag in the main table indicates
// that the action is a link to the overflow area. The LOOK flag indicates a
// lookahead pattern. The SOFT flag, in the overflow area, represents one of
// two things. Without the LOOK flag, it represents a close bracket rule that
// is skipped if the bracket doesn't match the most recent unmatched open
// bracket. With the LOOK flag, it represents a lookahead which only applies if
// there is a non-empty current token.
enum { LINK = 0x80, SOFT = 0x80, LOOK = 0x40, FLAGS = 0xC0 };

// Given a state machine table for a language and an initial state, scan the
// given array of characters, usually a line, fill in its style bytes in the
// output array. Carry out bracket matching as appropriate using the given
// stack. If the array of state names is not NULL, use it to print a trace of
// the execution. Return the final state.
int scan(byte *table, int s0, char *in, byte *out, byte *stack, char **names);
