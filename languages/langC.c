#include "lang.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// C language definition, based on the C18 standard. The source text is assumed
// to be normalised, with no control characters other than \n and no digraphs or
// trigraphs. See https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2310.pdf,
// Appendix A, or the grammar at https://csijh.github.io/pecan/c/

// TODO: handle local stuff via bracket matching (so local brackets are handled
// properly, and so start delimiters of unclosed strings etc are mismatched)
// TODO: consider bad newline.
// TODO: get number() right (see pecan)
// TODO: remove semicolon before scanning
// TODO: BadDelim (so warnings can be lifted)
// TODO: bracket handling.

// Extra tags, to recognise specific symbols temporarily for tracking contexts
// or handling ambiguity.
enum extraTag {
    Eq=nTags, Dot, Arrow, Less, Greater, Enum, Struct, Hash, Define, Elif, Else,
    Endif, Error, Ifdef, Ifndef, If, Include, Line, Pragma, Start, Undef,
    Join
};

// These are their spellings, for testing.
static char *names[] = {
    "=", ".", "->", "<", ">", "enum", "struct", "#", "define", "elif", "else",
    "endif", "error", "ifdef", "ifndef", "if", "include", "line", "pragma",
    "start", "undef", "\\"
};

// Display a token for testing.
static void show(token t, char *out) {
    if (t.tag >= nTags) showToken(t, names[t.tag - nTags], out);
    else showToken(t, NULL, out);
}

// The info provided for a token with a fixed spelling is its name and tag.
typedef struct fixedInfo { char *name; int tag; } fixedInfo;

// All the tokens with fixed spellings; keywords, operators, signs, delimiters,
// in lexicographic order except for prefixes, and with a sentinel at the end.
static fixedInfo fixed[] = {
    {"!=",InOp}, {"!",PreOp}, {"\"",Quotes}, {"##",InSign}, {"#",Hash},
    {"%=",InOp}, {"%",InOp}, {"&&",InOp}, {"&=",InOp}, {"&",InOp}, {"'",Quote},
    {"(",Open}, {")",Close}, {"*/",StopC}, {"*=",InOp}, {"*",InOp}, {"++",Op},
    {"+=",InOp}, {"+",InOp}, {",",InSign}, {"--",Op}, {"-=",InOp}, {"->",Arrow},
    {"-",InOp}, {"...",Sign}, {"/*",StartC}, {"//",Note}, {"/=",InOp},
    {"/",InOp}, {":",InSign}, {";",InSign}, {"<<=",InOp}, {"<<",InOp},
    {"<=",InOp}, {"<",Less}, {"==",InOp}, {"=",Eq}, {">=",InOp}, {">>=",InOp},
    {">>",InOp}, {">",Greater}, {"?",InOp}, {"[",Open1}, {"]",Close1},
    {"^=",InOp}, {"^",InOp}, {"_Alignas",Key}, {"_Alignof",Key},
    {"_Atomic",Type}, {"_Bool",Type}, {"_Complex",Type}, {"_Generic",Type},
    {"_Imaginary",Type}, {"_Noreturn",Key}, {"_Static_assert",Key},
    {"_Thread_Local",Key}, {"alignof",Key}, {"auto",Key}, {"bool",Type},
    {"break",Key}, {"case",Key}, {"char",Type}, {"const",Key}, {"continue",Key},
    {"default",Key}, {"define",Define}, {"double",Type}, {"do",Key},
    {"elif",Elif}, {"else",Else}, {"endif",Endif}, {"enum",Enum},
    {"error",Error}, {"extern",Key}, {"false",Key}, {"float",Type}, {"for",Key},
    {"goto",Key}, {"ifdef",Ifdef}, {"ifndef",Ifndef}, {"if",If},
    {"include",Include}, {"inline",Key}, {"int",Type}, {"line",Line},
    {"long",Type}, {"pragma",Pragma}, {"register",Key}, {"restrict",Key},
    {"return",Key}, {"short",Type}, {"signed",Type}, {"sizeof",Key},
    {"start",Start}, {"static",Key}, {"struct",Struct}, {"switch",Key},
    {"true",Key}, {"typedef",Key}, {"undef",Undef}, {"union",Key},
    {"unsigned",Type}, {"void",Type}, {"volatile",Type}, {"while",Key},
    {"{",BeginC}, {"|=",InOp}, {"||",InOp}, {"|",InOp}, {"}",EndC}, {"~",PreOp},
    {"",Bad}
};
enum { nFixed = sizeof(fixed) / sizeof(fixedInfo) };

// A hash table for looking up fixed tokens. The hash function is simply the
// first char (if < 128).
static short fixedTable[128];

// Fill in the table.
static void fillFixedTable() {
    int ch = 0;
    for (int i = 0; i < nFixed; i++) {
        int start = fixed[i].name[0];
        for ( ; ch <= start; ch++) fixedTable[ch] = i;
    }
}

// -----------------------------------------------------------------------------

// Look in the text for an upper case or lower case letter or underscore. Also
// accept \ if followed by U or u, or a byte with high bit set.
static bool letter(char const *s) {
    unsigned char ch = s[0];
    if ('a' <= ch && ch <= 'z') return true;
    if ('A' <= ch && ch <= 'Z') return true;
    if (ch == '_' || (ch & 0x80) != 0) return true;
    if (ch == '\\' && s[1] == 'U') return true;
    if (ch == '\\' && s[1] == 'u') return true;
    return false;
}

// Look for a digit.
static bool digit(char const *s) {
    return ('0' <= s[0] && s[0] <= '9');
}

// Look for a letter or digit.
static bool alpha(char const *s) {
    if ('0' <= s[0] && s[0] <= '9') return true;
    return letter(s);
}

// Look for a kex character.
static bool hex(char const *s) {
    if ('0' <= s[0] && s[0] <= '9') return true;
    if ('A' <= s[0] && s[0] <= 'F') return true;
    if ('a' <= s[0] && s[0] <= 'f') return true;
    return false;
}

// -----------------------------------------------------------------------------
// Scanning functions. Each function recognises and returns a token. If
// unsuccessful, it returns a zero-length token.

// Look for a fixed token from the table. If it is a keyword, check whether it
// is followed by a letter or digit in which case it is an identifier.
static token lookup(char const *s) {
    unsigned char ch = s[0];
    if ((ch & 0x80) != 0) return (token) {Gap, 0};
    int start = fixedTable[ch];
    for (int i = start; ch == fixed[i].name[0]; i++) {
        int n = strlen(fixed[i].name);
        if (strncmp(s, fixed[i].name, n) != 0) continue;
        if (letter(s) && alpha(s+n)) {
            while (alpha(s+n)) n++;
            if (n > 255) n = 255;
            return (token) {Id, n};
        }
        int tag = fixed[i].tag;
        return (token) {tag, n};
    }
    return (token) {Gap, 0};
}

// Scan a gap. The source text is normalised, so only spaces are relevant.
static token gap(char const *s) {
    int n = 0;
    while (s[n] == ' ') n++;
    if (n > 255) n = 255;
    return (token) {Gap, n};
}

// Scan a newline.
static token newline(char const *s) {
    int n = 0;
    if (s[0] == '\n') n++;
    return (token) {Newline, n};
}

// Scan a number, with possible exponents. Also recognise a single dot.
static token number(char const *s) {
    int n = 0;
    if (s[0] == '.' && (s[1] < '0' || s[1] > '9')) {
        return (token) {Dot, 1};
    }
    while (true) {
        if ('0' <= s[n] && s[n] <= '9') n++;
        else if (s[n] == '.') n++;
        else if (s[n] == 'e' && digit(s+n+1)) n += 1;
        else if (s[n] == 'E' && digit(s+n+1)) n += 1;
        else if (s[n] == 'p' && digit(s+n+1)) n += 1;
        else if (s[n] == 'P' && digit(s+n+1)) n += 1;
        else if (s[n] == 'e' && s[n+1] == '+') n += 2;
        else if (s[n] == 'e' && s[n+1] == '-') n += 2;
        else if (s[n] == 'E' && s[n+1] == '+') n += 2;
        else if (s[n] == 'E' && s[n+1] == '-') n += 2;
        else if (s[n] == 'p' && s[n+1] == '+') n += 2;
        else if (s[n] == 'p' && s[n+1] == '-') n += 2;
        else if (s[n] == 'P' && s[n+1] == '+') n += 2;
        else if (s[n] == 'P' && s[n+1] == '-') n += 2;
        else break;
    }
    if (n > 255) n = 255;
    return (token) {Value, n};
}

// Scan an identifier.
static token identifier(char const *s) {
    int n = 0;
    if (letter(s)) {
        n++;
        while (alpha(s+n)) n++;
    }
    if (n > 255) n = 255;
    if (s[n] == '(') return (token) {Function, n};
    else return (token) {Id, n};
}

// Scan an escape sequence.
static token escape(char const *s) {
    if (s[0] != '\\') return (token) {Escape, 0};
    int n = 1;
    if (s[1] == '\n') return (token) {Join, 1};
    if ('0' <= s[1] && s[1] <= '7') {
        while (n < 4 && '0' <= s[n] && s[n] <= '7') n++;
    }
    else if (s[1] == 'x') {
        n = 2;
        while (hex(s+n)) n++;
    }
    else if (s[1] == 'u') {
        for (n = 2; n < 6; n++) {
            if (! hex(s+n)) return (token) { Bad, n };
        }
    }
    else if (s[1] == 'U') {
        for (n = 2; n < 10; n++) {
            if (! hex(s+n)) return (token) { Bad, n };
        }
    }
    else switch (s[1]) {
        case 'a': case 'b': case 'f': case 'n': case 'r': case 't':
        case 'v': case '"': case '\'': case '\\':
            n = 2; break;
            default: return (token) {Bad, 2};
    }
    return (token) {Escape, n};
}

// Scan a bad character.
static token other(char const *s) {
    if ((s[0] & 0x80) == 0) return (token) {Bad, 1};
    int n = 1;
    while ((s[n] & 0x80) != 0) n++;
    if (n > 255) n = 255;
    return (token) {Bad, n};
}

static token scanToken(char const *s) {
    token t = lookup(s); if (t.length > 0) return t;
    if (letter(s)) return identifier(s);
    if (digit(s) || s[0] == '.') return number(s);
    if (s[0] == ' ') return gap(s);
    if (s[0] == '\n') return newline(s);
    if (s[0] == '\\') return escape(s);
    return other(s);
}

// Scan a line of text into tokens. The last token is Newline.
static int scanTokens(char const *s, token *out) {
    int n = 0, tag = Gap;
    while (tag != Newline) {
        token t = scanToken(s);
        out[n++] = t;
        s += t.length;
        tag = t.tag;
    }
    return n;
}

// -----------------------------------------------------------------------------
// Context sensitive adjustment (micro-parsing) is done using a state machine.
// Each state has a function with a switch on the current token's tag. The join
// flag is added to one of the other states to indicate that two lines are
// joined, so semicolon and indent handling are not applicable.
enum states {
    StartS, CharS, StringS, NoteS, PropertyS, HashS, HashIncludeS, FileS,
    StructS, EqS, EqOpenS, EqOpenIdS, EqOpenStructS, CurlyS,
    JoinS = 32
};

// Change Id to Function. Needs lookahead.
static void adjustFunction(token *t) {
    token *next = t + 1;
    if (next->tag == Gap) next = next + 1;
    if (next->tag == Open) t->tag = Function;
}

// If '... is unclosed, insert a zero-length misquote before the newline.
static void adjustBadQuote(token *t) {
    token *u = t + 1;
    *u = *t;
    *t = (token) { .tag = Misquote, .length=0 };
}

// If "... is unclosed, insert a zero-length MisQuotes before the newline.
static void adjustBadQuotes(token *t) {
    token *u = t + 1;
    *u = *t;
    *t = (token) { .tag = Misquotes, .length=0 };
}

// TODO: state includes position of opening delimiter?

// In the start state, look for ambiguous cases and for the tokens
// . -> ( # include  ' " // = struct enum
static int startState(int state, token *t) {
    switch (t->tag) {
        case Eq: t->tag = InOp; state = EqS; break;
        case Dot: t->tag = InOp; state = PropertyS; break;
        case Arrow: t->tag = InOp; state = PropertyS; break;
        case Less: t->tag = InOp; break;
        case Greater: t->tag = InOp; break;
        case Enum: t->tag = Key; state = StructS; break;
        case Struct: t->tag = Key; state = StructS; break;
        case Hash: t->tag = Reserved; state = HashS; break;
        case Quote: state = CharS; break;
        case Quotes: state = StringS; break;
        case Note: t->tag = Commented; state = NoteS; break;
        case Id: adjustFunction(t); break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: break;
        case Define: case Elif: case Endif: case Error: case Ifdef: case Ifndef:
        case Include: case Line: case Pragma: case Start: case Undef:
            t->tag = Id; break;
        case If: case Else:
            t->tag = Key; break;
        default: break;
    }
    return state;
}

// In the char state (after ') make tokens Quoted. Tag StartC or StopC as Bad.
// Leave escapes. If reach Newline, tag last token as Bad.
static int charState(int state, token *t) {
    switch (t->tag) {
        case Escape: break;
        case StartC: t->tag = Bad; break;
        case StopC: t->tag = Bad; break;
        case Quote: state = StartS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: adjustBadQuote(t); state = StartS; break;
        default: t->tag = Quoted; break;
    }
    return state;
}

// In the string state (after ") make tokens inside Quoted. Tag StartC or StopC
// as Bad. Leave escapes. If reach Newline, find open quote and tag it as Bad.
// If reach Join, carry the state over to the next line.
static int stringState(int state, token *t) {
    switch (t->tag) {
        case Escape: break;
        case StartC: t->tag = Bad; break;
        case StopC: t->tag = Bad; break;
        case Quotes: state = StartS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: adjustBadQuotes(t); state = StartS; break;
        default: t->tag = Quoted; break;
    }
    return state;
}

// In the note state (after //) make tokens inside Commented. Tag StartC or
// StopC as Bad.
static int noteState(int state, token *t) {
    switch (t->tag) {
        case StartC: t->tag = Bad; break;
        case StopC: t->tag = Bad; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: state = StartS; break;
        default: t->tag = Commented; break;
    }
    return state;
}

// In the property state (after . or ->) fix the next token.
static int propertyState(int state, token *t) {
    switch (t->tag) {
        case Id: t->tag = Property; state = StartS; break;
        default: state = startState(StartS,t); break;
    }
    return state;
}

// In the hash state (after #) fix the next token.
static int hashState(int state, token *t) {
    switch (t->tag) {
        case Include: t->tag = Reserved; state = HashIncludeS; break;
        case Define: case Elif: case Endif: case Error: case Ifdef: case Ifndef:
        case Line: case Pragma: case Start: case Undef:
        case If: case Else:
            t->tag = Reserved; state = StartS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        default: t->tag = Bad; state = StartS; break;
    }
    return state;
}

// In the hash include state (after #include) look for a filename.
static int hashIncludeState(int state, token *t) {
    switch (t->tag) {
        case Less: t->tag = Quote; state = FileS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        default: state = startState(StartS,t); break;
    }
    return state;
}

// In the file state (after #include <) look for a filename.
static int fileState(int state, token *t) {
    switch (t->tag) {
        case Greater: t->tag = Quote; state = StartS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: adjustBadQuote(t); state = StartS; break;
        default: t->tag = Quoted; break;
    }
    return state;
}

// The state after seeing struct or enum changes a following { from BeginC to
// OpenC and looks for "struct s" or "enum e".
static int structState(int state, token *t) {
    switch (t->tag) {
        case BeginC: t->tag = OpenC; state = StartS; break;
        case Id: state = CurlyS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: break;
        default: state = startState(StartS,t); break;
    }
    return state;
}

// The state after "=" looks for "= {" or "= (x) {" or "= (struct x) {".
static int eqState(int state, token *t) {
    switch (t->tag) {
        case BeginC: t->tag = OpenC; state = StartS; break;
        case Open: state = EqOpenS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: break;
        default: state = startState(StartS,t); break;
    }
    return state;
}

// The state after "= (" looks for Id or "struct"
static int eqOpenState(int state, token *t) {
    switch (t->tag) {
        case Id: state = EqOpenIdS; break;
        case Struct: t->tag = Key; state = EqOpenStructS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: break;
        default: state = startState(StartS,t); break;
    }
    return state;
}

// The state after "= (struct" looks for Id
static int eqOpenStructState(int state, token *t) {
    switch (t->tag) {
        case Id: state = EqOpenIdS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: break;
        default: state = startState(StartS,t); break;
    }
    return state;
}

// The state after "= (x" or "= (struct x" looks for ).
static int eqOpenIdState(int state, token *t) {
    switch (t->tag) {
        case Close: state = CurlyS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: break;
        default: state = startState(StartS,t); break;
    }
    return state;
}

// The curly state represents a context such as "struct x" etc. which expects
// curly brackets which aren't block brackets, but which can't be extended
// further before the brackets. Change { from OpenB to OpenC. During bracket
// matching } is changed to CloseC to indicate that a following semicolon is
// needed.
static int curlyState(int state, token *t) {
    switch (t->tag) {
        case BeginC: t->tag = OpenC; state = StartS; break;
        case Join: t->tag = Sign; state = state + JoinS; break;
        case Newline: break;
        default: state = startState(StartS,t); break;
    }
    return state;
}

// In any of the join states, the only token left to process is the newline.
// Carry the state past it.
static int joinState(int state, token *t) {
    switch (t->tag) {
        case Newline: break;
    }
    return state;
}

// Adjust a line of tokens, given the state carried over from the previous line.
// Skip indents and gaps, which have no effect. Return the final state, to be
// carried over to the next line.
static int adjust(int state, token *ts) {
    for (token *t = &ts[0]; ; t = t + 1) {
        if (t->tag == Gap) continue;
        switch (state) {
            case StartS: state = startState(state, t); break;
            case CharS: state = charState(state, t); break;
            case StringS: state = stringState(state, t); break;
            case NoteS: state = noteState(state, t); break;
            case PropertyS: state = propertyState(state, t); break;
            case HashS: state = hashState(state, t); break;
            case HashIncludeS: state = hashIncludeState(state, t); break;
            case FileS: state = fileState(state, t); break;
            case StructS: state = structState(state, t); break;
            case EqS: state = eqState(state, t); break;
            case EqOpenS: state = eqOpenState(state, t); break;
            case EqOpenIdS: state = eqOpenIdState(state, t); break;
            case EqOpenStructS: state = eqOpenStructState(state, t); break;
            case CurlyS: state = curlyState(state, t); break;
            default: state = joinState(state, t); break;
        }
        if (t->tag == Newline) break;
    }
    return state;
}

// Remove the join flag from the previous line, then build the tokens.
int scanC(int state, char const *s, token *out) {
    if (state >= JoinS) state = state - JoinS;
    scanTokens(s, out);
    state = adjust(state, out);
    return state;
}

// -----------------------------------------------------------------------------
// Bracket matching. Forwards and backwards, depending on cursor.
// (a) tell which tokens are (open/close) brackets.
// (b) match and mismatch brackets.
// (c) add "starts/end in comment" flags to each line for display
// (d) extend bracketing to line (or cursor) FB
// (e) prepare line for display.
// An edit to a line can change 1) prev semi 2) indent 3) state 4) flags
// LINE: position, (length), state0, (state1), tokens, (toklen), comment flags

// -----------------------------------------------------------------------------

#ifdef TEST

// Check fixed tokens are in lexicographic order, except for prefixes.
static void testFixed() {
    for (int i = 0; i < nFixed - 1; i++) {
        char *x = fixed[i].name;
        char *y = fixed[i+1].name;
        int xlen = strlen(x), ylen = strlen(y);
        bool less = strcmp(x, y) < 0;
        bool prefix = (xlen < ylen) && strncmp(x, y, xlen) == 0;
        bool suffix = (xlen > ylen) && strncmp(x, y, ylen) == 0;
        bool ok = ! prefix && (less || suffix);
        if (! ok) printf("Strings %s and %s out of order\n", x, y);
        assert(ok);
    }
}

// Compare actual with expected.
static bool check(char *expect, token *ts) {
    char out[100] = "\0";
    for (int i = 0; ; i++) {
        show(ts[i], out);
        if (ts[i].tag == Newline) break;
    }
    bool result = strcmp(out, expect) == 0;
    if (! result) {
        printf("expect: %s", expect);
        printf("actual: %s", out);
    }
    return result;
}

// Make a writable copy of s0 in s, then convert ` to " and $ to \. (Using ` and
// $ in tests allows the strings to be lined up.)
static void prepare(char *s, char *s0) {
    strcpy(s, s0);
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '`') s[i] = '\"';
        if (s[i] == '$') s[i] = '\\';
    }
}

// Test raw scanning, without context sensitive scanning.
static bool test1(char *in0, char *out0) {
    char in[100], out[100];
    token ts[200];
    prepare(in, in0);
    prepare(out, out0);
    scanTokens(in, ts);
    return check(out, ts);
}

// Test full scanning, with initial and final states. Make sure all the custom
// tags have been resolved.
static bool test2(int state0, int state1, char *in0, char *out0) {
    char in[100], out[100];
    token ts[200];
    prepare(in, in0);
    prepare(out, out0);
    int state = scanC(state0, in, ts);
    if (state != state1) printf("bad state (%d!=%d)\n", state, state1);
    int ok = true;
    for (int i = 0; ts[i].tag != Newline; i++) {
        if (ts[i].tag < nTags) continue;
        ok = false;
        printf("Tag %s not resolved\n", names[ts[i].tag - nTags]);
    }
    return check(out, ts) && state == state1 && ok;
}

// Test the result of calling scanTokens, without the context phase.
static void testPre() {
    assert(test1(
        "abc\n",               // identifier
        "I~~\n"));
    assert(test1(
        "(1.2e3)\n",           // brackets and numbers
        "(V~~~~)\n"));
    assert(test1(
        "while (b) n++;\n",    // keyword
        "K~~~~ (I) IO~:\n"));
    assert(test1(
        "if (b) { n = 1; }\n",  // ambiguous keyword ("if" or "#if")
        "if (I) < I = V: >\n"));
    assert(test1(
        "int n;\n",             // type-keyword
        "T~~ I:\n"));
    assert(test1(
        "string s;\n",          // user defined type, treated as identifier
        "I~~~~~ I:\n"));
    assert(test1(
        "enum x { Cb };\n",     // {} tagged as block brackets for now
        "enum I < I~ >:\n"));
    assert(test1(
        "int ns[] = { 1, 2, 34};\n",   // ditto
        "T~~ I~[] = < V: V: V~>:\n"));
    assert(test1(
        "char *s = `a$nb$0c$04d`;\n",      // escape sequences recognised
        "T~~~ oI = `I$~I$~I$~~I`:\n"));
    assert(test1(
        "char *s = `$037e$038f$xffga`\n",  // more escape sequences
        "T~~~ oI = `$~~~I$~~VI$~~~I~`\n"));
    assert(test1(
        "*/ ` // `; /* abc ` */ ` // \n",   // quote, note, comment
        "^~ ` #~ `: %~ I~~ ` ^~ ` #~ \n"));
}

// Test the result of scanning, including the context phase.
static void testPost() {
    assert(test2(StartS, StartS,    // blank line
        "\n",
        "\n"));
    assert(test2(StartS, StartS,    // id
        "abc\n",
        "I~~\n"));
    assert(test2(StartS, StartS,    // infix minus
        "x-y\n",
        "IoI\n"));
    assert(test2(StartS, StartS,    // prefix minus
        "x+-y+(-z)\n",
        "IooIo(oI)\n"));
    assert(test2(StartS, StartS,    // prefix increment, reported as nonfix
        "(++n)\n",
        "(O~I)\n"));
    assert(test2(StartS, StartS,    // increment at start of line (nonfix)
        "++\n",
        "O~\n"));
    assert(test2(StartS, StartS,    // postfix increment (nonfix)
        "n++\n",
        "IO~\n"));
    assert(test2(StartS, StartS,    // properties
        "s.x + p->y\n",
        "IoP o Io~P\n"));
    assert(test2(StartS, StartS,    // functions
        "f() g ()\n",
        "F() F ()\n"));
    assert(test2(StartS, StartS,    // define as a reserved word
        "#define\n",
        "RR~~~~~\n"));
    assert(test2(StartS, StartS,    // define after a gap
        "#  define\n",
        "R  R~~~~~\n"));
    assert(test2(StartS, StartS,    // define as an id
        "int define\n",
        "T~~ I~~~~~\n"));
    assert(test2(StartS, StartS,    // else as reserved word
        "#else\n",
        "RR~~~\n"));
    assert(test2(StartS, StartS,    // else as keyword
        "if(b);else;\n",
        "K~(I):K~~~:\n"));
    assert(test2(StartS, StartS,    // include
        "#include\n",
        "RR~~~~~~\n"));
    assert(test2(StartS, StartS,    // include file
        "#include <f>\n",
        "RR~~~~~~ 'Q'\n"));
    assert(test2(StartS, StartS,    // include file incomplete
        "#include <f\n",            // (zero-length Misquote added)
        "RR~~~~~~ 'Q?\n"));
    assert(test2(StartS, StartS,    // block brackets
        "if (b) {}\n",
        "K~ (I) <>\n"));
    assert(test2(StartS, StartS,    // block brackets
        "struct {}\n",              // (open bracket converted to OpenC)
        "K~~~~~ {>\n"));
    assert(test2(StartS, StructS,   // struct state carried over ...
        "struct\n",
        "K~~~~~\n"));
    assert(test2(StructS, StartS,   // ... and affects next line
        "{}\n",
        "{>\n"));
    assert(test2(StartS, StartS,    // struct id {}
        "struct s {}\n",
        "K~~~~~ I {>\n"));
    assert(test2(StartS, StartS,    // = {}
        "x = {}\n",
        "I o {>\n"));
    assert(test2(StartS, StartS,    // = (s) {}
        "x = (s) {}\n",
        "I o (I) {>\n"));
    assert(test2(StartS, StartS,    // = (struct s) {}
        "x = (struct s) {}\n",
        "I o (K~~~~~ I) {>\n"));
    assert(test2(StartS, StartS,    // string TODO line up.
        "s = `abc`;\n",
        "I o `Q~~`:\n"));
    assert(test2(StartS, StartS,    // char
        "s = 'a';\n",
        "I o 'Q':\n"));
    assert(test2(StartS, JoinS+StartS,    // join lines TODO: line up
        "int$\n",
        "T~~S\n"));
    assert(test2(StartS, JoinS+StringS,   // join strings and ...
        "`abc$\n",
        "`Q~~S\n"));
    assert(test2(JoinS+StringS, StartS,    // ...string unclosed
        "def\n",                           // (0-length Misquotes added)
        "Q~~?\n"));
    assert(test2(StartS, JoinS+CharS,      // join char and ...
        "s = 'a$\n",
        "I o 'QS\n"));
    assert(test2(JoinS+CharS, StartS,      // ... end on next line
        "'\n",
        "'\n"));
    assert(test2(StartS, JoinS+NoteS,      // join notes...
        "//abc$\n",
        "C~C~~S\n"));
    assert(test2(JoinS+NoteS, StartS,      // ... onto next line
        "def\n",
        "C~~\n"));
}

void testLangC() {
    fillFixedTable();
    testFixed();
    testPre();
    testPost();
    printf("langC module OK\n");
}

#endif

#ifdef TESTlangC

int main() {
    testLangC();
    return 0;
}

#endif
