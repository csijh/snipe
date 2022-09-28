#include "lang.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// C language definition, based on the C11 standard. See
// http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1548.pdf. The source text is
// assumed to be normalised, with no control characters other than \n and no
// digraphs or trigraphs. Backslash newline is only supported between tokens. A
// few ids such as bool, false, true from <stdbool.h> are treated as keywords.

// The info provided for a token with a fixed spelling is its name and tag.
typedef struct fixedInfo { char *name; int tag; } fixedInfo;

// All the tokens with fixed spellings; keywords, operators, signs, delimiters,
// in lexicographic order except for prefixes, and with a sentinel at the end.
static fixedInfo fixed[] = {
    {"!=",InOp}, {"!",PreOp}, {"\"",Quote}, {"##",InSign}, {"%=",InOp},
    {"%",InOp}, {"&&",InOp}, {"&=",InOp}, {"&",InOp}, {"(",Open0}, {")",Close0},
    {"*/",CloseC}, {"*=",InOp}, {"*",InOp}, {"++",PrePostOp}, {"+=",InOp},
    {"+",PreInOp}, {",",InSign}, {"--",PrePostOp}, {"-=",InOp}, {"->",InSign},
    {"-",PreInOp}, {"...",Sign}, {"/*",OpenC}, {"//",Note}, {"/=",InOp},
    {"/",InOp}, {":",InSign}, {";",InSign}, {"<<=",InOp}, {"<<",InOp},
    {"<=",InOp}, {"<",InOp}, {"==",InOp}, {"=",InSign}, {">=",InOp},
    {">>=",InOp}, {">>",InOp}, {">",InOp}, {"?",InOp}, {"[",Open1},
    {"]",Close1}, {"^=",InOp}, {"^",InOp}, {"_Alignas",Key},  {"_Atomic",Type},
    {"_Bool",Type}, {"_Complex",Type}, {"_Generic",Type}, {"_Imaginary",Type},
    {"_Noreturn",Key}, {"_Static_assert",Key}, {"_Thread_Local",Key},
    {"alignof",Key}, {"auto",Key}, {"bool",Type}, {"break",Key}, {"case",Key},
    {"char",Type}, {"const",Key}, {"continue",Key}, {"default",Key},
    {"double",Type}, {"do",Key}, {"else",Key}, {"enum",Key}, {"extern",Key},
    {"false",Key}, {"float",Type}, {"for",Key}, {"goto",Key}, {"if",Key},
    {"inline",Key}, {"int",Type}, {"long",Type}, {"register",Key},
    {"restrict",Key}, {"return",Key}, {"short",Type}, {"signed",Type},
    {"sizeof",Key}, {"static",Key}, {"struct",Key}, {"switch",Key},
    {"true",Key}, {"typedef",Key}, {"union",Key}, {"unsigned",Type},
    {"void",Type}, {"volatile",Type}, {"while",Key}, {"{",OpenB}, {"|=",InOp},
    {"||",InOp}, {"|",InOp}, {"}",CloseB}, {"~",PreOp}, {"\x7f",Bad}
};
enum { nFixed = sizeof(fixed) / sizeof(fixedInfo) };

// A hash table for looking up fixed tokens. The hash function is simply the
// first char (if < 127).
static char fixedTable[128];

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
// accept \ if followed by U or u, or a byte with with high bit set.
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

// -----------------------------------------------------------------------------

// Look for a fixed token from the table. If it is a keyword, check whether it
// is followed by a letter or digit (and if so, reject it because it is an
// identifier). Return a token with length 0 if unsuccessful.
static token lookup(char const *s) {
    unsigned char ch = s[0];
    if (ch >= 127) return (token) {Gap, 0};
    int start = fixedTable[ch];
    for (int i = start; ch == fixed[i].name[0]; i++) {
        int n = strlen(fixed[i].name);
        if (strncmp(s, fixed[i].name, n) != 0) continue;
        if (letter(s) && alpha(s+n)) continue;
        return (token) {fixed[i].tag, n};
    }
    return (token) {Gap, 0};
}

// Scan a gap. The source text is normalised, so only spaces are relevant.
static token gap(char const *s) {
    int n = 0;
    while (s[n] == ' ') n++;
    return (token) {Gap, n};
}

// Scan a newline (assumed active).
static token newline(char const *s) {
    int n = 0;
    if (s[0] == '\n') n++;
    return (token) {Newline, n};
}

// Scan a number, with possible exponents.
static token number(char const *s) {
    int n = 0;
    if (s[0] == '.' && (s[1] < '0' || s[1] > '9')) {
        return (token) {Value, 0};
    }
    while (true) {
        if ('0' <= s[n] && s[n] <= '9') n++;
        else if (s[n] == '.') n++;
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
    return (token) {Value, n};
}

// Scan a character literal, without checking the length or the correctness of
// any escapes.
static token character(char const *s) {
    int n = 0;
    if (s[0] == '\'') {
        while (true) {
            if (s[n] == '\\' && s[n+1] == '\'') n = n + 2;
            else if (s[n] == '\n') return (token) {Quote, n};
            else if (s[n] != '\'') n++;
            else break;
        }
    }
    return (token) {Quote, n};
}

// Scan an identifier.
static token identifier(char const *s) {
    int n = 0;
    if (letter(s)) {
        n++;
        while (alpha(s+n)) n++;
    }
    if (s[n] == '(') return (token) {Function, n};
    else return (token) {Id, n};
}

// Scan a joiner or a bad character.
static token other(char const *s) {
    if (s[0] == '\\' && s[1] == '\n') return (token) {Endline, 2};
    int n = 1;
    unsigned char *bs = (unsigned char *)s;
    while ((bs[n] & 0x80) != 0) n++;
    return (token) {Bad, n};
}

// Scan any token in the normal state.
static token outToken(char const *s) {
    token m;
    m = lookup(s); if (m.length > 0) return m;
    if (letter(s)) return identifier(s);
    if (digit(s)) return number(s);
    if (s[0] == ' ') return gap(s);
    if (s[0] == '\n') return newline(s);
    if (s[0] == '\'') return character(s);
    return other(s);
}

// // Recognise a token while in in a // note. Report comment delimiters as
// // warnings.
// static token noteToken(char const *s) {
//     token m = outToken(s);
//     switch (m.tag) {
//         case Gap: m.tag = NoteGap; break;
//         case DOpen: m.tag = NoteBad; break;
//         case DClose: m.tag = NoteBad; break;
//         case Newline: break;
//         default: m.tag = Note; break;
//     }
//     return m;
// }
//
// // Scan a token inside a string literal. Report comment delimiters as warnings.
// static token quoteToken(char const *s) {
//     token m = outToken(s);
//     switch (m.tag) {
//         case Gap: m.tag = QuoteGap; break;
//         case DOpen: m.tag = QuoteBad; break;
//         case DClose: m.tag = QuoteBad; break;
//         case Quote: m.tag = QuoteEnd; break;
//         default: m.tag = Quote; break;
//     }
//     return m;
// }
//
// // Currently, ignore the state argument. (It should record whether a following
// // open curly is to be OpenB or OpenC.)
void scanC(int state, char const *s, token *out) {
     int n = 0, tag = Gap;
     while (tag != Newline && tag != Endline) {
         token t = outToken(s);
         printf("t %d %d\n", t.tag, t.length);
         out[n++] = t;
         s += t.length;
         tag = t.tag;
     }
     out[n++] = (token) {0,0};
// //    if (state == InNote) return noteToken(s);
// //    else if (state == InQuote) return quoteToken(s);
// //    else return outToken(s);
//     // TODO context.
}

// =============================================================================

#ifdef TEST

static char *names[] = {
    [Bad]="B", [Warn]="W", [Gap]=" ", [Note]="N", [Quote]="Q",
    [Value]="V", [Type]="T", [Key]="K", [Reserved]="R", [Id]="I",
    [Function]="F", [Property]="P", [Newline]=".", [Endline]="E",
    [PreOp]="L", [InOp]="O", [PostOp]="R", [PreInOp]="X", [PrePostOp]="Y",
    [Sign]="S", [PreSign]="!", [InSign]="|", [PostSign]="?", [Open0]="(",
    [Close0]=")", [Open1]="[", [Close1]="]", [Open2]="{", [Close2]="}",
    [OpenB]="<", [CloseB]=">", [OpenC]="^", [CloseC]="$", [OpenQ]="o",
    [CloseQ]="c",
};

// Check fixed tokens are in lexicographic order, except for prefixes. Check
// that char can be used for indexes into the array.
static void testFixed() {
    for (int i = 0; i < nFixed - 1; i++) {
        char *x = fixed[i].name;
        char *y = fixed[i+1].name;
        int xlen = strlen(x), ylen = strlen(y);
        bool less = strcmp(x, y) < 0;
        bool prefix = (xlen < ylen) && strncmp(x, y, xlen) == 0;
        bool suffix = (xlen > ylen) && strncmp(x, y, ylen) == 0;
        bool ok = ! prefix && (less || suffix);
        if (! ok) printf("Fixed error on %s and %s\n", x, y);
        assert(ok);
    }
    assert(nFixed < 128);
}

static bool check(char *in, char *expect) {
    token ts[strlen(in) + 1];
    char out[100] = "\0";
    scanC(0, in, ts);
    for (int i = 0; ts[i].length > 0; i++) {
        sprintf(out + strlen(out), "%s", names[ts[i].tag]);
        for (int j = 1; j < ts[i].length; j++) {
            sprintf(out + strlen(out), "-");
        }
    }
    bool result = strcmp(out, expect) == 0;
    if (! result) {
        printf("expect: %s\n", expect);
        printf("actual: %s\n", out);
    }
    return result;
}

// Each test consists of two strings, the input, and matching output which
// lines up. In the input, " is represented as ` and \ as $ to avoid escapes.
static char *tests[] = {
    // "abc\n",
    // "I--.",
    // "(def)\n",
    // "(I--).",
    // "if (b) n = 1;\n",
    // "K- (I) I | V|.",
    // "if (b) { n = 1; }\n",
    // "K- (I) < I | V| >.",
    // // Type-keyword followed by id.
    // "int n;\n",
    // "T-- I|.",
    // // Programmer defined type followed by id.
    // "string s;\n",
    // "I----- I|.",
    // "enum suit { Club, Diamond, Heart, Spade };\n",
    // "K--- I--- < I---| I------| I----| I---- >|.",
    // "int ns[] = { 1, 2, 34};\n",
    // "T-- I-[] | < V| V| V->|.",
    // String quotes and escapes (Use ` for " so the strings line up)
    "char *s = `a$nb$0c$04d`;\n",
    "T--- OI | QI?-I?-I?--IqS.",
    // Three-digit and two-digit octal escape, hex escape.
    "char *s = \"\037e\038f\xffga\"\n",
    "K---_OI_S_\"?---I?--VI?---I\".",


};
enum { nTests = sizeof(tests) / sizeof(char *) };

static void testScan() {
    char in[100];
    for (int i = 0; i < nTests; i += 2) {
        strcpy(in, tests[i]);
        for (int i = 0; i < strlen(in); i++) {
            if (in[i] == '`') in[i] = '"';
            if (in[i] == '$') in[i] = '\\';
        }
        char *expect = tests[i+1];
        assert(check(in, expect));
    }
}

void testLangC() {
    fillFixedTable();
    testFixed();
    testScan();
    printf("c module OK\n");
}

#endif

#ifdef TESTlangC

int main() {
    testLangC();
    return 0;
}

#endif
