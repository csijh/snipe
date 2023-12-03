// Snipe editor. Free and open source, see licence.txt.

// The table has a row per state, and a column per character in \n, \s, !..~.
// The columns have a fixed width. A cell has two bytes containing an action.
enum { WIDTH = 96, CELL = 2 };

// The first byte in a cell has these flags. In the main body of the table, the
// link flag indicates that the cell is a link to the overflow area. In the
// overflow area, the same flag represents a soft bracket match. In any cell
// without a link flag, the look flag indicates a lookahead pattern which does
// not consume input.
enum { FLAGS = 0xC0, LOOK = 0x40, LINK = 0x80, SOFT = 0x80 };
static inline bool look(byte *cell) { return (cell[0] & LOOK) != 0; }
static inline bool link(byte *cell) { return (cell[0] & LINK) != 0; }
static inline bool soft(byte *cell) { return (cell[0] & SOFT) != 0; }

// In the case of a link, the two bytes of a cell form an offset to a list of
// patterns in the overflow area at the end of the table. The list consists of
// all the patterns starting with the current character. If there is no link,
// the cell represents a single one-character pattern.
static inline
int offset(byte *action) { return ((action[0] & ~LINK) << 8) + action[1]; }

// In the overflow area, a pattern consists of a length byte, the characters
// after the first (which has already been matched) and a two-byte cell
// containing the action. A list of patterns always ends with a one-character
// pattern which is certain to match, so the list needs no termination.
static inline int patternLength(byte *pattern) { return pattern[0]; }

// In the main body of the table if there is no link flag, or in the overflow
// area, the two bytes of a cell hold a type and a target state. The pair
// represent the action to take on the current character, i.e. to tag the
// current token with the type if it is not None, move past the pattern if the
// lookahead flag is not set, and go into the target state. If the type
// indicates an open bracket token, the bracket is pushed on the bracket stack.
// If it indicates a close bracket token, ....
static inline int type(byte *cell) { return cell[0] & ~FLAGS; }
static inline int target(byte *cell) { return cell[1]; }

// A pattern matches if its characters appear next in the input and, for a close
// bracket, if it matches the open bracket on top of the stack.
// TODO matchTop.

// Scan a line of text, using a language-specific table, and a start state,
// using an array of type bytes for output, and a stack for bracket matching.
// It is assumed that the arrays are pre-allocated to be big enough.
int scan(byte *table, int state, char *line, byte *out, Bracket *stack) {
    int n = strchr(line, '\n') + 1;
    for (int i = 0; i < n; i++) out[i] = None;
    int at = 0, start = 0;
    while (at < n) {
        char ch = line[at];
        int col = (ch == '\n') ? 0 : ch - ' ' + 1;
        byte *action = &table[CELL * (WIDTH * state + col)];
        int len = 1;
        if ((action[0] & LINK) != 0) {
            byte *p = table + offset(action);
            bool found = false;
            while (! found) {
                found = true;
                len = patternLength(p);
                for (int i = 1; i < len && found; i++) {
                    if (line[at + i] != p[i]) found = false;
                }
                int t = p[len] & ~FLAGS;
                if (found) {

                    if (endType(t) && ! match(top(out,at),t)) {
                        out[start] = t;
                        found = false;
                    }
                }
                if (found) action = p + len;
                else p = p + len + 2;
            }
        }
        bool lookahead = (action[0] & LOOK) != 0;
        int type = action[0] & TYPE;
        int target = action[1];
        if (tracing) trace(state, lookahead, line, at, len, type, states);
        if (! lookahead) at = at + len;
        if (type != None && start < at) {
            if (type != Miss) out[start] = type;
            if (beginType(type)) push(out, start);
            else if (endType(type)) pop(out, start);
            start = at;
        }
        state = target;
    }
    return state;
}
