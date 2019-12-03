// Generate or update tables of Unicode properties or tests in a given file. If
// any of the Unicode tables handled by this program are found in the file,
// their contents are replaced. For example, if the file has a line containing
// "categoryIndex[] = {" then lines between that and one containing just "};"
// are replaced by the general category index table.

// The data files used are taken from:
// www.unicode.org/Public/12.0.0/ucd/
// www.unicode.org/Public/12.0.0/ucd/auxiliary/
// www.unicode.org/Public/emoji/12.0/

// Some of the tables generated are two-stage tables. The use of multi-stage
// tables is described in Chapter 5 of the Unicode standard. There is an index
// table which maps a code/256 to one of the 256-byte blocks in a block table.
// Only the distinct blocks are stored in the block table, which reduces the
// size compared to a 1114112-entry full table.



// The second describes the grapheme break
// property of a code point, in tables graphemeIndex, graphemeTable, .

// TODO: add Bidi_Class (23), Bidi_Paired_Bracket, Bidi_Paired_Bracket_Type and
// XIDStart/XIDContinue. And NFC normalization.

// A table of test data is generated in utest.c for testing grapheme boundary
// breaks.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef unsigned char byte;

// ----- Table descriptions ---------------------------------------------------
// For each property table handled by this program, there is an array containing
// the text introducing the tables which this program will recognize and
// replace, the names of the Unicode data files the property is extracted from,
// and the relevant columns in the data files. There is also an enumeration of
// property constants suitable for copying, and an array of property names as
// they appear in the data files.

// ----------------------------------------------------------------------------
// The General Category of a code point.

char *Gc[] = {
    "categoryIndex[] = {", "categoryBlocks[] = {",
    "UnicodeData.txt", "2"};

enum GcValues {
    Cc, Cf, Cn, Co, Cs, Ll, Lm, Lo, Lt, Lu, Mc, Me, Mn, Nd, Nl, No, Pc, Pd, Pe,
    Pf, Pi, Po, Ps, Sc, Sk, Sm, So, Zl, Zp, Zs
};

char *GcNames[] = {
    [Cc]="Cc", [Cf]="Cf", [Cn]="Cn", [Co]="Co", [Cs]="Cs", [Ll]="Ll", [Lm]="Lm",
    [Lo]="Lo", [Lt]="Lt", [Lu]="Lu", [Mc]="Mc", [Me]="Me", [Mn]="Mn", [Nd]="Nd",
    [Nl]="Nl", [No]="No", [Pc]="Pc", [Pd]="Pd", [Pe]="Pe", [Pf]="Pf", [Pi]="Pi",
    [Po]="Po", [Ps]="Ps", [Sc]="Sc", [Sk]="Sk", [Sm]="Sm", [So]="So", [Zl]="Zl",
    [Zp]="Zp", [Zs]="Zs"
};
int GcCount = sizeof(GcNames) / sizeof(char *);

// ----------------------------------------------------------------------------
// The Grapheme Break property of a code point, suitable for finding the
// boundaries of extended grapheme clusters with a state machine. The property
// is described in table 2 of tr29 of the Unicode standard, plus emoji data as
// in tr51. The GbValues enumeration, graphemeState enumeration, and
// graphemeStateTable can all be copied.

char *Gb[] = {
    "graphemeIndex[] = {", "graphemeTable[] = {",
    "GraphemeBreakProperty.txt", "1", "emoji-data.txt", "1"};

enum GbValues {
    CR, LF, CO, EX, ZW, RI, PR, SM, HL, HV, HT, LV, LT, EP, OR
};

char *GbNames[] = {
    [CR]="CR", [LF]="LF", [CO]="Control", [EX]="Extend", [ZW]="ZWJ",
    [RI]="Regional_Indicator", [PR]="Prepend", [SM]="SpacingMark", [HL]="L",
    [HV]="V", [HT]="T", [LV]="LV", [LT]="LVT", [EP]="Extended_Pictographic",
    [OR]="Other"
};
int GbCount = sizeof(GbNames) / sizeof(char *);

enum graphemeState {
    S, C, E, R, P, L, V, T, J, Z, B = 0x10,
};

// FSM transition table for grapheme boundaries. The states are S = start, C =
// seen CR, E = looking for extenders, R = seen one regional indicator, P =
// prepending, L/V/T = seen Hangul L/V/T, J = in emoji, Z = seen emoji + ZWJ.
const byte graphemeStateTable[][15] = {
//CR   LF   CO   EX   ZW   RI   PR   SM   HL   HV   HT   LV   LT   EP   OR
{B|C, B|S, B|S, B|E, B|E, B|R, B|P, B|E, B|L, B|V, B|T, B|V, B|T, B|J, B|E},//S
{B|C,   S, B|S, B|E, B|S, B|R, B|P, B|E, B|L, B|V, B|T, B|V, B|T, B|J, B|E},//C
{B|C, B|S, B|S,   E,   E, B|R, B|P,   E, B|L, B|V, B|T, B|V, B|T, B|J, B|E},//E
{B|C, B|S, B|S,   E,   S,   E, B|P,   E, B|L, B|V, B|T, B|V, B|T, B|J, B|E},//R
{B|C, B|S, B|S,   E,   E,   R,   P,   E,   L,   V,   T,   V,   T,   J,   S},//P
{B|C, B|S, B|S,   E,   E, B|R, B|P,   E,   L,   V, B|T,   V,   T, B|J, B|E},//L
{B|C, B|S, B|S,   E,   E, B|R, B|P,   E, B|L,   V,   V, B|V, B|T, B|J, B|E},//V
{B|C, B|S, B|S,   E,   E, B|R, B|P,   E, B|L, B|V,   T, B|V, B|T, B|J, B|E},//T
{B|C, B|S, B|S,   J,   Z, B|R, B|P,   E, B|L, B|V, B|T, B|V, B|T, B|J, B|E},//J
{B|C, B|S, B|S,   J,   J, B|R, B|P,   E, B|L, B|V, B|T, B|V, B|T,   J, B|E},//Z
};

// ----------------------------------------------------------------------------
// The grapheme break tests, to test the state machine algorithm. Each test
// consists of an array of numbers. The first is the line number from the test
// file to help track down issues. The second is the number of code points in
// the test. The rest are code points with break indicators before, between and
// after. An indicator is 0 for no break and 1 for a break.

char *Gt[] = {
    "graphemeTests[][16] = {", NULL,
    "GraphemeBreakTest.txt", "0"};

// ----------------------------------------------------------------------------
// The bidirectional class of a code point.

char *Bi[] = {
    "bidiIndex[] = {", "bidiBlocks[] = {",
    "UnicodeData.txt", "4"};

enum biValues {
    BiL, BiR, BiEN, BiES, BiET, BiAN, BiCS, BiB, BiS, BiWS, BiON, BiBN, BiNSM,
    BiAL, BiLRO, BiRLO, BiLRE, BiRLE, BiPDF, BiLRI, BiRLI, BiFSI, BiPDI
};

char *BiNames[] = {
    [BiL]="L", [BiR]="R", [BiEN]="EN", [BiES]="ES", [BiET]="ET", [BiAN]="AN",
    [BiCS]="CS", [BiB]="B", [BiS]="S", [BiWS]="WS", [BiON]="ON", [BiBN]="BN",
    [BiNSM]="NSM", [BiAL]="AL", [BiLRO]="LRO", [BiRLO]="RLO", [BiLRE]="LRE",
    [BiRLE]="RLE", [BiPDF]="PDF", [BiLRI]="LRI", [BiRLI]="RLI", [BiFSI]="FSI",
    [BiPDI]="PDI"
};
int BiCount = sizeof(BiNames) / sizeof(char *);

// ----- Arrays ---------------------------------------------------------------

// Allocate new array. Precede by item size, capacity and length. Reserve 4
// ints, so result is pointer-aligned.
void *new(int max, int stride) {
    int *mem = malloc(4*sizeof(int) + max*stride);
    int *a = mem + 4;
    a[-3] = stride;
    a[-2] = max;
    a[-1] = 0;
    return a;
}

// Double the capacity.
void *grow(void *va) {
    int *a = (int *) va;
    a[-2] = 2 * a[-2];
    int *mem = &a[-4];
    mem = realloc(mem, 4*sizeof(int) + a[-2]*a[-3]);
    return &mem[4];
}

void release(void *a) { free((int *) a - 4); }

int stride(void *a) { return ((int *) a)[-3]; }

int max(void *a) { return ((int *) a)[-2]; }

int length(void *a) { return ((int *) a)[-1]; }

void *setLength(void *a, int n) {
    while (max(a) < n) a = grow(a);
    ((int *) a)[-1] = n;
    return a;
}

// Add a string to an array. Return possibly reallocated array.
char **addString(char *a[], char *s) {
    int n = length(a);
    a = setLength(a, n + 1);
    a[n] = s;
    return a;
}

// Find the index of a string in an array, or return -1.
int find(char *a[], char *s) {
    for (int i = 0; i < length(a); i++) {
        fflush(stdout);
        if (strcmp(a[i],s) == 0) return i;
    }
    return -1;
}

// ----- Files ----------------------------------------------------------------

// Read the content of a file into an array.
char *readFile(char const *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) { printf("Can't open %s\n", path); exit(1); }
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *content = new(length+1, 1);
    fread(content, length, 1, fp);
    fclose(fp);
    content[length] = '\0';
    return content;
}

// In the given file, replace the table with the given name.
void print(char const *file, char *name, byte *table) {
    char *old = readFile(file);
    char *start = strstr(old, name);
    if (start == NULL) { release(old); return; }
    while(*start != '\n') start++;
    start++;
    char *end = strstr(start, "};");
    if (end == NULL) { printf("Can't find end of %s\n", name); exit(1); }
    FILE *fp = fopen(file, "w");
    fprintf(fp, "%.*s", (int)(start - old), old);
    for (int i = 0; i < length(table); i++) {
        if (i > 0 && i%16 == 0) fprintf(fp, "\n");
        if (i%16 == 0) fprintf(fp, "   ");
        fprintf(fp, " %2d,", table[i]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "%s", end);
    fclose(fp);
    release(old);
}

// ----- Lines and fields -----------------------------------------------------

// Split text into an array of lines.
char **splitLines(char *s) {
    char **lines = new(128, sizeof(char *));
    char *line = s;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\r') s[i] = '\0';
        if (s[i] != '\n') continue;
        s[i] = '\0';
        lines = addString(lines, line);
        line = &s[i+1];
    }
    return lines;
}

// Check if a line is a comment.
bool comment(char *line) {
    return line[0] == '#' || line[0] == '\0';
}

// Extract the first code of a range from a line. Codes needs to be extracted
// from a line before spliiting the line into fields.
int firstCode(char *s) {
    return strtol(s, NULL, 16);
}

// Extract the last code of a range from a line, before splitting lines into
// fields. If not a range, repeat the first code. If the data file is
// UnicodeData.txt, the end of the range is on the next line. See testCodes.
int lastCode(char *s) {
    char *p = strstr(s, "..");
    if (p != NULL) return strtol(p+2, NULL, 16);
    p = strstr(s, "First");
    if (p == NULL) return strtol(s, NULL, 16);
    char *next = s + strlen(s) + 1;
    p = strstr(next, "Last");
    if (p == NULL) return strtol(s, NULL, 16);
    return strtol(next, NULL, 16);
}

// Split a line into fields by semicolons, and hash for a comment, and trim the
// fields.
char **splitFields(char *s, char *fields[]) {
    setLength(fields, 0);
    int start = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\t') s[i] = ' ';
        if (s[i] != ';' && s[i] != '#') continue;
        s[i] = '\0';
        for (int j = i-1; j > start && s[j] == ' '; j--) s[j] = '\0';
        while (s[start] == ' ') start++;
        fields = addString(fields, &s[start]);
        start = i+1;
    }
    addString(fields, &s[start]);
    return fields;
}

// Split tokens at spaces or tabs.
char **splitTokens(char *s, char *tokens[]) {
    setLength(tokens, 0);
    int start = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\t') s[i] = ' ';
        if (s[i] != ' ') continue;
        s[i] = '\0';
        for (int j = i-1; j > start && s[j] == ' '; j--) s[j] = '\0';
        while (s[start] == ' ') start++;
        tokens = addString(tokens, &s[start]);
        start = i+1;
    }
    addString(tokens, &s[start]);
    return tokens;
}

// ----- Two stage tables -----------------------------------------------------

// Split a line into fields, extract one, look up the code(s) in the values
// array, and put the value in the given full table.
void fillFromLine(char *line, int column, char **values, byte *full) {
    int start = firstCode(line);
    int end = lastCode(line);
    char **fields = new(16, sizeof(char *));
    fields = splitFields(line, fields);
    char *field = fields[column];
    int value = find(values, field);
    if (value < 0) printf("Can't find %s\n", field);
    assert(value >= 0 && value < 256);
    for (int code = start; code <= end; code++) full[code] = value;
    release(fields);
}

// Split a line into fields, extract one, check the value aganst the given one,
// and put the value for the range code(s) in the given full table.
void fillValueFromLine(char *line, int column, int v, char *value, byte *full) {
    int start = firstCode(line);
    int end = lastCode(line);
    char **fields = new(16, sizeof(char *));
    fields = splitFields(line, fields);
    char *field = fields[column];
    if (strcmp(value, field) == 0) {
        for (int code = start; code <= end; code++) full[code] = v;
    }
    release(fields);
}

// Convert a full table into an index table and block table. Assume the length
// of the index is already set to 1114112/256. Return the blocks table, which
// is reallocated as necessary.
byte *stage(byte *full, byte *index, byte *blocks) {
    setLength(blocks, 0);
    for (int b = 0; b < 1114112; b = b + 256) {
        int block = -1;
        for (int ob = 0; block < 0 && ob < length(blocks); ob = ob + 256) {
            if (memcmp(&full[b], &blocks[ob], 256) == 0) {
                block = ob;
            }
        }
        if (block < 0) {
            block = length(blocks);
            if (block / 256 >= 256) {
                fprintf(stderr, "More than 255 distinct blocks!\n");
                exit(1);
            }
            blocks = setLength(blocks, length(blocks) + 256);
            memcpy(&blocks[block], &full[b], 256);
        }
        index[b / 256] = block / 256;
    }
    return blocks;
}

// ----- Build tables ---------------------------------------------------------

// Buid the two-stage General Category tables and insert in given file. The
// default is Cn.
void buildCategories(char const *file) {
    char *source = Gc[2];
    int column = atoi(Gc[3]);
    char *text = readFile(source);
    char **lines = splitLines(text);
    byte *fullTable = new(1114112, 1);
    setLength(fullTable, 1114112);
    for (int i = 0; i < 1114112; i++) fullTable[i] = Cn;
    char **values = new(GcCount, sizeof(char *));
    for (int i = 0; i < GcCount; i++) values = addString(values, GcNames[i]);
    for (int i = 0; i < length(lines); i++) {
        if (comment(lines[i])) continue;
        fillFromLine(lines[i], column, values, fullTable);
    }
    byte *index = new(1114112/256, sizeof(byte));
    setLength(index, 1114112/256);
    byte *blocks = new(256, sizeof(byte));
    blocks = stage(fullTable, index, blocks);
    print(file, Gc[0], index);
    print(file, Gc[1], blocks);
    release(index);
    release(blocks);
    release(values);
    release(fullTable);
    release(lines);
    release(text);
}

// Buid the two-stage Grapheme Break tables and insert. The entries come from a
// main data file and an emoji data file.
void buildGraphemes(char const *file) {
    char *source = Gb[2];
    int column = atoi(Gb[3]);
    char *text = readFile(source);
    char **lines = splitLines(text);
    byte *fullTable = new(1114112, 1);
    setLength(fullTable, 1114112);
    for (int i = 0; i < 1114112; i++) fullTable[i] = OR;
    char **values = new(GbCount, sizeof(char *));
    for (int i = 0; i < GbCount; i++) values = addString(values, GbNames[i]);
    for (int i = 0; i < length(lines); i++) {
        if (comment(lines[i])) continue;
        fillFromLine(lines[i], column, values, fullTable);
    }
    release(text);
    release(lines);
    text = readFile(Gb[4]);
    column = atoi(Gb[5]);
    lines = splitLines(text);
    for (int i = 0; i < length(lines); i++) {
        if (comment(lines[i])) continue;
        fillValueFromLine(lines[i], column, EP, values[EP], fullTable);
    }
    byte *index = new(1114112/256, sizeof(byte));
    setLength(index, 1114112/256);
    byte *blocks = new(256, sizeof(byte));
    blocks = stage(fullTable, index, blocks);
    print(file, Gb[0], index);
    print(file, Gb[1], blocks);
    release(index);
    release(blocks);
    release(values);
    release(fullTable);
    release(lines);
    release(text);
}

void buildGraphemeTests(char const *file) {
    char *old = readFile(file);
    char *start = strstr(old, Gt[0]);
    if (start == NULL) { release(old); return; }
    while(*start != '\n') start++;
    start++;
    char *end = strstr(start, "};");
    if (end == NULL) { printf("Can't find end of tests\n"); exit(1); }
    FILE *fp = fopen(file, "w");
    fprintf(fp, "%.*s", (int)(start - old), old);
    char *text = readFile(Gt[2]);
    char **lines = splitLines(text);
    char **fields = new(16, sizeof(char *));
    char **tokens = new(16, sizeof(char *));
    int *numbers = new(16, sizeof(int));
    for (int i = 0; i < length(lines); i++) {
        if (comment(lines[i])) continue;
        setLength(fields, 0);
        fields = splitFields(lines[i], fields);
        char *field = fields[atoi(Gt[3])];
        tokens = splitTokens(field, tokens);
        numbers = setLength(numbers, 2 + length(tokens));
        numbers[0] = i + 1;
        numbers[1] = length(tokens) / 2;
        for (int j = 0; j < length(tokens); j++) {
            if (strncmp("÷", tokens[j], 2) == 0) numbers[2+j] = 1;
            else if (strncmp("×", tokens[j], 2) == 0) numbers[2+j] = 0;
            else numbers[2+j] = strtol(tokens[j], NULL, 16);
        }
        fprintf(fp, "    {");
        for (int j = 0; j < length(numbers) - 1; j++) {
            fprintf(fp, " %d,", numbers[j]);
        }
        fprintf(fp, " 1 },\n");
    }
    fprintf(fp, "    { -1, 0, 1 },\n");
    fprintf(fp, "%s", end);
    fclose(fp);
    release(numbers);
    release(tokens);
    release(fields);
    release(lines);
    release(text);
    release(old);
}

void buildBidis(char const *file) {
    char *source = Bi[2];
    int column = atoi(Bi[3]);
    char *text = readFile(source);
    char **lines = splitLines(text);
    byte *fullTable = new(1114112, 1);
    setLength(fullTable, 1114112);
    for (int i = 0; i < 1114112; i++) fullTable[i] = BiL;
    char **values = new(BiCount, sizeof(char *));
    for (int i = 0; i < BiCount; i++) values = addString(values, BiNames[i]);
    for (int i = 0; i < length(lines); i++) {
        if (comment(lines[i])) continue;
        fillFromLine(lines[i], column, values, fullTable);
    }
    byte *index = new(1114112/256, sizeof(byte));
    setLength(index, 1114112/256);
    byte *blocks = new(256, sizeof(byte));
    blocks = stage(fullTable, index, blocks);
    print(file, Bi[0], index);
    print(file, Bi[1], blocks);
    release(index);
    release(blocks);
    release(values);
    release(fullTable);
    release(lines);
    release(text);
}

// ----- Testing --------------------------------------------------------------

void testArrays() {
    char *ws[] = {"one", "two", "three"};
    char **a = new(1, sizeof(char *));
    for (int i = 0; i < 3; i++) a = addString(a, ws[i]);
    assert(length(a) == 3 && max(a) == 4);
    assert(a[0] == ws[0] && a[1] == ws[1] && a[2] == ws[2]);
    release(a);
}

void testSplitLines() {
    char s[] = "a\nbc\ndef\n";
    char *ls[3] = {"a", "bc", "def"};
    char **a = splitLines(s);
    assert(length(a) == 3);
    for (int i = 0; i < 3; i++) assert(strcmp(a[i], ls[i])==0);
    release(a);
}

void testSplitFields() {
    char s[] = "a;  bc  ;def\n";
    char *fs[3] = {"a", "bc", "def"};
    char **a = new(16, sizeof(char *));
    a = splitFields(s, a);
    assert(length(a) == 3);
    for (int i = 0; i < 3; i++) assert(strcmp(a[i], fs[i])==0);
    release(a);
}

void testCodes() {
    assert(firstCode("03C0;x") == 0x3C0 && firstCode("03C0..03CF;x") == 0x3C0);
    assert(lastCode("03C0;x") == 0x3C0 && lastCode("03C0..03CF;x") == 0x3CF);
    char s1[] =
        "3400;<CJK Ideograph Extension A, First>;Lo;0;L;;;;;N;;;;;\0"
        "4DB5;<CJK Ideograph Extension A, Last>;Lo;0;L;;;;;N;;;;;\0";
    assert(lastCode(s1) == 0x4DB5);
    char s2[] =
        "100000;<Plane 16 Private Use, First>;Co;0;L;;;;;N;;;;;\0"
        "10FFFD;<Plane 16 Private Use, Last>;Co;0;L;;;;;N;;;;;\0";
    assert(lastCode(s2) == 0x10FFFD);
}

int main(int n, char const *args[]) {
    testArrays();
    testSplitLines();
    testCodes();
    if (n != 2) {
        fprintf(stderr, "Use: ./unigen file\n");
        exit(1);
    }
    buildCategories(args[1]);
    buildGraphemes(args[1]);
    buildGraphemeTests(args[1]);
    buildBidis(args[1]);
}
