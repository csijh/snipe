// Unicode support. Free and open source, see licence.txt.
#include <stdbool.h>

// Provide general category lookup for code points. Provide iteration through
// code points of UTF-8 text, with grapheme boundaries. The lookup tables are
// inserted into this module automatically by unigen.c, from Unicode data files.
// The current version of the Unicode standard supported is 12.0.0.

// Categories in the order used in the lookup tables.
enum category {
    Cc, Cf, Cn, Co, Cs, Ll, Lm, Lo, Lt, Lu, Mc, Me, Mn, Nd, Nl, No, Pc, Pd, Pe,
    Pf, Pi, Po, Ps, Sc, Sk, Sm, So, Zl, Zp, Zs
};

// Look up the category of a code point.
int ucategory(int code);

// The unicode replacement code point for all invalid UTF-8 sequences.
enum { UBAD = 0xFFFD };

// Check if string is UTF-8 valid. If nullCheck is true, also check for nulls.
bool uvalid(int n, char s[n], bool nullCheck);

// The code and byte-length of a UTF-8 code point, plus grapheme boundary info.
struct codePoint { int code; unsigned char length, grapheme; };
typedef struct codePoint codePoint;

// Get a codePoint structure suitable for iterating through a string.
codePoint getCodePoint();

// Get the next code point. The first call should be on a grapheme boundary,
// after which grapheme boundaries will be tracked.
void nextCode(codePoint *cp, const char *s);

// Call only on a grapheme boundary. Returns the
codePoint getGrapheme(const char *s);

// Check if the most recent code point is the start of a grapheme.
bool graphemeStart(char grapheme);

enum bidi {
    BiL, BiR, BiEN, BiES, BiET, BiAN, BiCS, BiB, BiS, BiWS, BiON, BiBN, BiNSM,
    BiAL, BiLRO, BiRLO, BiLRE, BiRLE, BiPDF, BiLRI, BiRLI, BiFSI, BiPDI
};
