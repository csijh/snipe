// The Snipe editor is free and open source. See licence.txt.

bool isOpener(int type) {
    return FirstB <= type && type <= LastB;
}

bool isCloser(int type) {
    return FirstE <= type && type <= LastE;
}

bool match(int opener, int closer) {
    return closer == opener + FirstE - FirstB;
}

// The state transition table for a language comes from ../languages. The table
// has one row per state. There are 98 columns, two for \n or \s according to
// whether or not there is a non-empty current token, and one each for !..~.
// A cell has two bytes containing an action.
enum { WIDTH = 98, CELL = 2 };

static inline int columnOf(char ch, bool token) {
    if (ch == \n) return token ? 0 : 1;
    if (ch == ' ') return token ? 2 : 3;
    return ch - '!' + 4;
}

static inline int cellOf(int r, int c) {
    return CELL * (r * WIDTH + c);
}

// The first byte in a cell may have these flags. In the main body of the table,
// the link flag indicates that the cell is a link to the overflow area. In the
// overflow area, the same flag represents a soft bracket match. In any cell
// which isn't a link, the look flag indicates a lookahead pattern which does
// not consume input.
enum { FLAGS = 0xC0, LINK = 0x80, SOFT = 0x80, LOOK = 0x40 };
static inline bool isLink(byte *cell) { return (cell[0] & LINK) != 0; }
static inline bool isSoft(byte *cell) { return (cell[0] & SOFT) != 0; }
static inline bool isLook(byte *cell) { return (cell[0] & LOOK) != 0; }

// In the case of a link, the two bytes of a cell form an offset to a list of
// patterns in the overflow area at the end of the table. The list consists of
// all the patterns starting with the current character. In the main body of
// the table, if a cell is not a link, it is a single one-character pattern.
static inline int offset(byte *c) { return ((c[0] & ~LINK) << 8) + c[1]; }

// In a cell which isn't a link, the two bytes of a cell hold a type and a
// target state. The pair represent the action to take on the current
// character, i.e. if the type is not None then mark the current token with it,
// move past the pattern if the lookahead flag is not set, carry out any
// bracket matching, and go into the target state.
static inline int typeOf(byte *cell) { return cell[0] & ~FLAGS; }
static inline int targetOf(byte *cell) { return cell[1]; }

// In the overflow area, a pattern consists of a length byte, the characters
// after the first (which has already been matched) and a two-byte cell
// containing the action. A list of patterns always ends with a one-character
// pattern which is certain to match, so the list needs no termination.
static inline byte *patternCell(byte *p) { return &p[p[0]]; }

// For a soft closing bracket pattern, check for a match with the top opener on
// the stack. (If there is no match, the pattern is skipped.)
static inline bool matchTop(int *stack, byte *types, int closer) {
    int n = length(stack);
    if (n == 0) return false;
    int opener = types[stack[n-1]];
    return match(opener, closer);
}

// Push the position of an opening bracket on the stack (assuming enough room).
static inline void push(int *stack, int atOpener) {
    int n = length(stack);
    adjust(stack, +1);
    stack[n-1] = atOpener;
}

// Pop an opener from the stack, mark it and the closer as Bad if mismatched.
static inline void pop(int *stack, int atCloser, byte *types) {
    int n = length(stack);
    if (n == 0) { types[atCloser] |= Bad; return; }
    int atOpener = stack[n-1];
    adjust(stack, -1);
    if (match(types[atOpener], types[atCloser])) return;
    types[atOpener] |= Bad;
    types[atCloser] |= Bad;
}

void trace(char **ns, int s, bool look, char *in, int at, int n, int type) {
    printf("%-10s ", ns[s]);
    int pad = 10;
    if (look) { printf("\\"); pad--; }
    if (in[at] == '\\') { printf("\\"); pad--; }
    if (n == 1 && in[at] == ' ') printf("s");
    else if (n == 1 && in[at] == '\n') printf("n");
    else printf("%.*s", n, &in[at]);
    printf(".*s ", pad-n, "          ");
    printf(typeNames[type]);
}

int scan(byte *lang, int s0, char *in, int at, byte *out, int *stk, char **ns) {
    int n = length(in);
    for (int i = at; i < n; i++) out[i] = None;
    int state = s0;
    int start = 0;
    while (at < n) {
        char ch = in[at];
        int col = columnOf(ch, start < at);
        byte *cell = &table[cellOf(state, col)];
        int len = 1;
        if (isLink(cell)) {
            byte *pattern = table + offset(cell);
            bool found = false;
            while (! found) {
                found = true;
                len = pattern[0];
                cell = patternCell(pattern);
                for (int i = 1; i < len && found; i++) {
                    if (in[at + i] != pattern[i]) found = false;
                }
                int type = typeOf(cell);
                if (found && isSoft(cell)) {
                    int closer = typeOf(cell);
                    if (! matchTop(stack,out,closer)) found = false;
                }
                pattern = pattern + len + CELL;
            }
        }
        bool lookahead = isLook(cell);
        int type = typeOf(cell);
        int target = targetOf(cell);
        if (ns != NULL) trace(ns, state, lookahead, at, len, type, target);
        if (! lookahead) at = at + len;
        if (type != None && start < at) {
            out[start] = type;
            if (isOpener(type)) push(stack, start);
            else if (isCloser(type)) pop(stack, start, out);
            start = at;
        }
        if (ch == ' ') { out[at++] = Gap; start = at; }
        else if (ch == '\n') { out[at++] = Newline; start = at; }
        state = target;
    }
    return state;
}

// ---------- Testing ----------------------------------------------------------
#ifdef scanTest

int main() {
    printf("Scanning is tested in languages/compile.c\n");
}

#endif
