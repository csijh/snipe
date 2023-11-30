// Snipe editor. Free and open source, see licence.txt.

// The table has a row per state, and a column per character in \n, \s, !..~.
// The columns have this width and cell size. The number of rows is variable,
// depending on the number of states.
enum { WIDTH = 96, CELL = 2 };

// The first byte in a cell has flags to represent a link or a lookahead.
enum { LINK = 0x80, LOOK = 0x40, FLAGS = 0xC0 };

// If a cell in the main body of the table has no link flag, the two bytes are a
// type and a target state. It represents the action to take on the current
// character, namely to tag it with the type, move past if the lookahead flag
// is not set, and go into the target state.
static inline int type(byte *cell) { return cell[0] & NOFLAGS; }
static inline bool look(byte *cell) { return (cell[0] & LOOK) != 0; }
static inline int target(byte *cell) { return cell[1]; }

// In the case of a link, the two bytes of a cell form an offset to a list of
// patterns in an overflow area at the end of the table.
static inline
int offset(byte *action) {
    return ((action[0] & ~LINK) << 8) + action[1];
}

// A pattern consists of a length byte, the characters of the pattern after the
// first (which has already been matched) and a two-byte cell containing the
// action. A list of patterns always ends with a pattern which is certain to
// match, so the list needs no termination.
static inline int patternLength(byte *pattern) { return pattern[0]; }

// A pattern matches if its characters appear next in the input and, for a close
// bracket, if it matches the open bracket on top of the stack.
// TODO matchTop.

// Scan a line of text, using a language-specific table, and a start state,
// using an array of type bytes for output, and a stack for bracket matching.
// The type array and stack should be pre-allocated to be big enough.
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
