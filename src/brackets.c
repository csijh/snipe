struct bracket { int site; int partner; };
typedef struct bracket Bracket;

Bracket *newBrackets() {
    return newArray(sizeof(Bracket));
}

// Scan is: move gap to end of line, deleteLeft to start of line, scan.

void deleteLeft(int *brackets, int startOfLine) {
    // pop any brackets with site >= startOfLine
}

void moveGapLeft(int *brackets, int *types) {
    // if bracket = top opener, pop it, pop corresponder after gap (match?)
    // else is closer:
    //    push after gap
    //    search for opener, unmark, push after gap
}

// Search leftward for the opener which matches the given closer.
int findOpener(int closer, byte *types) {
    int depth = 1, i;
    for (i = closer-1; i >= 0 && depth > 0; i--) {
        if (isCloser(types[i])) depth++;
        else if (isOpener(types[i])) depth--;
    }
    // Check depth == 0; guaranteed? need MISSING constant
    // Unmark
    return i;
}

// Search is linear scan, matching brackets, looking for first unmatched.
// Don't need to skip stacked, because must be before the first.
// Possibly no more costly than maintaining links between matched brackets.
// Speeding up using indents is probably not worth it.

// If its top, pop it:      ['(',-9][TOP,-1]   ->   [TOP,-9]
// If matched, unmatch it:  ['(',+5]...[)',-5][TOP,-9]   ->   ['(',-9]...[TOP,-5]
void deleteLeft(Bracket *brackets, int *types) {
    int n = length(brackets);
    if (n < 2) return;
    int top = brackets[n-1].partner;
    int last = n-2;
    if (last == top) {
        brackets[last].partner = top;
//        unmark(types, brackets[last].site);
    }
    else {
        int opener = brackets[last].partner;
        unmark(types, brackets[opener].site);
//        unmark(types, brackets[last].site);
        brackets[opener].partner = top;
        top = opener;
        brackets[last].partner = top;
    }
    adjust(brackets, -1);
}

void moveLeft(Brackets *brackets, int *types) {
    int n = length(brackets);
}

// Consider search:
// Have closer. Want matching opener. Use closer/opener counts per line.
// Then search in the line.


// Contains all brackets.
// Site is index into (text and) types array.
// Partner contains matches, and doubles as stack.
// Where is the stack top held? Search (#c,#o) or special or separate?
// Current line must be cursor line.
// Operations:
//    (Move current site to end of line.)
//    Delete brackets from site1 to site2 (need max).
//    Allow scan to insert new brackets (matchTop, addOpener, addCloser)
//    Move current site to nextLine/prevLine/cursor.

// Case study: compile.c
// Bytes: 40840  (text and types 81680)
// Lines: 5938   (boundaries 23752)
// Brackets: (733+203+166)*2*8 = (brackets structure bytes: 17623)

// Suggestion: don't track matching of brackets at all.
//     just stacks of sites of unmatched openers/closers before/after cursor
// Matching of vertical brackets is indicated by indenting:
//     up indent = prev indent - closers before cursor
//     down indent = next indent - openers after cursor
// Matching on the line is:
//     mark spare openers before cursor
//     mark

// Deletion is: remove any markers inside range
// Scan is: add markers
// Move is:
