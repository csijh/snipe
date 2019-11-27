// Unicode support. Free and open source, see licence.txt.
#include <stdbool.h>

// Provide general category lookup for code points. Provide iteration through
// code points of UTF-8 text, with grapheme boundaries. The lookup tables are
// inserted into this module automatically by unigen.c, from Unicode data files.
// The current version of the Unicode standard supported is 12.0.0.

// TODO: add validity checking (then reject file)

// Categories in the order used in the lookup tables.
enum category {
    Cc, Cf, Cn, Co, Cs, Ll, Lm, Lo, Lt, Lu, Mc, Me, Mn, Nd, Nl, No, Pc, Pd, Pe,
    Pf, Pi, Po, Ps, Sc, Sk, Sm, So, Zl, Zp, Zs
};

// Look up the category of a code point.
int ucategory(int code);

// The unicode replacement code point for all invalid UTF-8 sequences.
enum { UBAD = 0xFFFD };

extern bool uvalid(char *s);

// The code and byte-length of a UTF-8 code point, plus grapheme boundary info.
struct codePoint { int code; unsigned char length, grapheme; };
typedef struct codePoint codePoint;

// Get the code point at the given position. If grapheme boundaries are
// required, call this only on the first code point of a grapheme.
codePoint getCode(char const *s);

// Get the next code point. If used in sequence, with the grapheme info from the
// previous code point, this will track grapheme boundaries.
codePoint nextCode(char grapheme, const char *s);

// Call only on a grapheme boundary. Returns the
codePoint getGrapheme(const char *s);

// Check if the most recent code point is the start of a grapheme.
bool graphemeStart(char grapheme);

// Grapheme break values. A classification of all code points for finding
// boundaries between graphemes (extended grapheme clusters). These are for
// internal use, but are provided here for unigen.c.
enum grapheme {
    CR, LF, CO, EX, ZW, RI, PR, SM, HL, HV, HT, LV, LT, EP, OR
};
