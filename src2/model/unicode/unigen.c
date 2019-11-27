// Generate unicode table files and grapheme break tests from the data files:
// www.unicode.org/Public/12.0.0/ucd/UnicodeData.txt
// www.unicode.org/Public/12.0.0/ucd/auxiliary/GraphemeBreakProperty.txt
// www.unicode.org/Public/emoji/12.0/emoji-data.txt
// www.unicode.org/Public/12.0.0/ucd/auxiliary/GraphemeBreakTest.txt

// Two two-stage tables are generated. The use of multi-stage tables is
// described in Chapter 5 of the Unicode standard. There is an index table which
// maps a code/256 to one of the 256-byte blocks in a data table. Only the
// distinct blocks are stored in the data table, which is what reduces the size
// compared to a 1114112-entry full table.

// One two-stage table describes the general category of a code point, in tables
// categoryIndex, categoryTable. The second describes the grapheme break
// property of a code point, in tables graphemeIndex, graphemeTable, suitable
// for finding the boundaries of extended grapheme clusters with a state
// machine. The grapheme class is as described in table 2 of tr29 of the Unicode
// standard, plus emoji data as in tr51.

// TODO: add Bidi_Class (23), Bidi_Paired_Bracket, Bidi_Paired_Bracket_Tyoe and
// XIDStart/XIDContinue.

// A table of test data is generated in utest.c for testing grapheme boundary
// breaks.

#include "../unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef unsigned char byte;

// Allocate large tables statically, not on the heap.
enum { MAX = 1114112 };
static byte fullTable[MAX];
static byte indexTable[MAX/256];
static byte dataTable[MAX];

// Read the content of a file into an array.
static char *readFile(char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) { printf("Can't open %s\n", path); exit(1); }
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *content = malloc(length + 1);
    fread(content, length, 1, fp);
    fclose(fp);
    content[length] = '\0';
    return content;
}

char *catNames[] = {
    [Cc]="Cc", [Cf]="Cf", [Cn]="Cn", [Co]="Co", [Cs]="Cs", [Ll]="Ll", [Lm]="Lm",
    [Lo]="Lo", [Lt]="Lt", [Lu]="Lu", [Mc]="Mc", [Me]="Me", [Mn]="Mn", [Nd]="Nd",
    [Nl]="Nl", [No]="No", [Pc]="Pc", [Pd]="Pd", [Pe]="Pe", [Pf]="Pf", [Pi]="Pi",
    [Po]="Po", [Ps]="Ps", [Sc]="Sc", [Sk]="Sk", [Sm]="Sm", [So]="So", [Zl]="Zl",
    [Zp]="Zp", [Zs]="Zs"
};

static int findCat(char *s) {
    for (int i = 0; i < 30; i++) {
        if (strcmp(s,catNames[i]) == 0) return i;
    }
    printf("Can't find cat name %s\n", s);
    exit(1);
}

// UnicodeData.txt contains codes in order, no comments. Each line contains:
//    code;name;category;...
// Ranges have two lines:
//    code;<..., First>;category;...
//    code;<..., Last>;category;...

// Fill the full category table from the unicode data file.
static void fillCatTable(char *path, byte *fullTable) {
    char *text = readFile(path);
    int gap = 0, gapType = Cn, n = strlen(text);
    char *code, *name, *cat;
    for (int p = 0; p < n; p++) {
        code = &text[p];
        while (text[p] != ';') p++;
        text[p++] = '\0';
        name = &text[p];
        while (text[p] != ';') p++;
        text[p++] = '\0';
        cat = &text[p];
        while (text[p] != ';') p++;
        text[p++] = '\0';
        while (text[p] != '\n') p++;
        int ch = strtol(code, NULL, 16);
        int type = findCat(cat);
        while (gap < ch) fullTable[gap++] = gapType;
        fullTable[ch] = type;
        gap = ch + 1;
        if (strcmp(&name[strlen(name)-8], ", First>") == 0) gapType = type;
        else gapType = Cn;
    }
    for (int i = gap; i < MAX; i++) fullTable[i] = gapType;
    free(text);
}

char *graphNames[] = {
    [CR]="CR", [LF]="LF", [CO]="Control", [EX]="Extend", [ZW]="ZWJ",
    [RI]="Regional_Indicator", [PR]="Prepend", [SM]="SpacingMark", [HL]="L",
    [HV]="V", [HT]="T", [LV]="LV", [LT]="LVT", [EP]="Extended_Pictographic",
    [OR]="Other"
};

static int findGraph(char *s) {
    for (int i = 0; i < 15; i++) {
        if (i == 14) printf("[%s][%s]\n", s, graphNames[i]);
        if (strcmp(s,graphNames[i]) == 0) return i;
    }
    printf("Can't find graphene name %s\n", s);
    exit(1);
}

// GraphemeBreakProperty.txt contains blank lines and # comments and property
// lines in property order. Each non-comment line contains:
//    code-or-range ; property ;...

// Fill the full grapheme property table from GraphemeBreakProperty.txt.
static void fillGraphTable(char *path, byte *fullTable) {
    char *text = readFile(path);
    for (int i = 0; i < MAX; i++) fullTable[i] = OR;
    int start, end, n = strlen(text);
    char *code, *endcode, *property;
    for (int p = 0; p < n; p++) {
        if (text[p] == '\n') continue;
        if (text[p] == '#') { while (text[p] != '\n') p++; continue; }
        code = &text[p];
        while (text[p] != ' ' && text[p] != '.') p++;
        if (text[p] == '.') endcode = &text[p + 2];
        else endcode = code;
        text[p++] = '\0';
        while (text[p] != ';') p++;
        p++;
        while (text[p] == ' ') p++;
        property = &text[p];
        while (text[p] != ' ' && text[p] != '#') p++;
        text[p++] = '\0';
        while (text[p] != '\n') p++;
        start = strtol(code, NULL, 16);
        end = strtol(endcode, NULL, 16);
        int value = findGraph(property);
        for (int i = start; i <= end; i++) fullTable[i] = value;
    }
    free(text);
}

// emoji-data.txt contains comments and property lines in property order. We are
// only interested in Extended_Pictographic, to override the grapheme property.
// Each non-comment line contains:
//    code-or-range ; property ;...

// Override the full grapheme property table from emoji-data.txt.
static void overrideGraphTable(char *path, byte *fullTable) {
    char *text = readFile(path);
    int start, end, n = strlen(text);
    char *code, *endcode, *property;
    for (int p = 0; p < n; p++) {
        if (text[p] == '\n') continue;
        if (text[p] == '#') { while (text[p] != '\n') p++; continue; }
        code = &text[p];
        while (text[p] != ' ' && text[p] != '.') p++;
        if (text[p] == '.') endcode = &text[p + 2];
        else endcode = code;
        text[p++] = '\0';
        while (text[p] != ';') p++;
        p++;
        while (text[p] == ' ') p++;
        property = &text[p];
        while (text[p] != ' ' && text[p] != '#') p++;
        text[p++] = '\0';
        while (text[p] != '\n') p++;
        if (strcmp(property, "Extended_Pictographic") != 0) continue;
        start = strtol(code, NULL, 16);
        end = strtol(endcode, NULL, 16);
        for (int i = start; i <= end; i++) {
            if (fullTable[i] != OR) {
                fprintf(stderr, "Expecting Other\n");
                exit(1);
            }
            fullTable[i] = EP;
        }
    }
    free(text);
}

// Compress the full table into the index/data tables, returning the data size.
static int pack(byte *fullTable, byte *indexTable, byte *dataTable) {
    int indexSize = 0, dataSize = 0;
    for (int b = 0; b < MAX; b = b + 256) {
        int address = -1;
        for (int ob = 0; ob < dataSize; ob = ob + 256) {
            if (memcmp(&fullTable[b], &dataTable[ob], 256) == 0) {
                address = ob / 256;
                break;
            }
        }
        if (address >= 0) {
            indexTable[indexSize++] = address;
            continue;
        }
        address = dataSize / 256;
        indexTable[indexSize++] = address;
        if (address >= 256) {
            fprintf(stderr, "Table too big!\n");
            exit(1);
        }
        for (int i=0; i<256; i++) {
            dataTable[dataSize+i] = fullTable[b+i];
        }
        dataSize = dataSize + 256;
    }
    return dataSize;
}

// In the given file, replace the table with the given name.
static void print(char *file, char *name, int n, byte table[n]) {
    char *old = readFile(file);
    char prefix[100];
    sprintf(prefix, "%s[] = {", name);
    char *start = strstr(old, prefix);
    if (start == NULL) { printf("Can't find %s\n", prefix); exit(1); }
    while(*start != '\n') start++;
    start++;
    char *end = strstr(start, "};");
    if (start == NULL) { printf("Can't find end of %s\n", name); exit(1); }
    FILE *fp = fopen(file, "w");
    fprintf(fp, "%.*s", (int)(start - old), old);
    for (int row = 0; row < n/16; row++) {
        fprintf(fp, "   ");
        for (int col = 0; col < 16; col++) {
            fprintf(fp, " %2d,", table[row*16+col]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "%s", end);
    fclose(fp);
    free(old);
}

// Read the tests and print them into the file, with a sentinel.
static void fillTests(char *file, char *tests) {
    char *old = readFile(file);
    char prefix[100];
    sprintf(prefix, "testTable[][16] = {");
    char *start = strstr(old, prefix);
    if (start == NULL) { printf("Can't find %s\n", prefix); exit(1); }
    while(*start != '\n') start++;
    start++;
    char *end = strstr(start, "};");
    if (start == NULL) { printf("Can't find end of tests\n"); exit(1); }
    FILE *fp = fopen(file, "w");
    fprintf(fp, "%.*s", (int)(start - old), old);
    char *text = readFile(tests);
    int n = strlen(text);
    int breaks[100], codes[100], length, line = 0;
    for (int p = 0; p < n; p++) {
        line++;
        if (text[p] == '\n') continue;
        if (text[p] == '#') { while (text[p] != '\n') p++; continue; }
        length = 0;
        while (text[p] != '#' && text[p] != '\n') {
            if (strncmp("÷", &text[p], 2) == 0) {
                breaks[length] = 1;
                p += 2;
            }
            else if (strncmp("×", &text[p], 2) == 0) {
                breaks[length] = 0;
                p += 2;
            }
            else {
                codes[length++] = strtol(&text[p], NULL, 16);
                while(text[p] != ' ') p++;
            }
            while(text[p] == ' ' || text[p] == '\t') p++;
        }
        fprintf(fp, "    { %d, %d,", line, length);
        for (int i=0; i<length; i++) {
            fprintf(fp, " %d, %d,", breaks[i], codes[i]);
        }
        fprintf(fp, " 1 },\n");
        while(text[p] != '\n') p++;
    }
    fprintf(fp, "    { -1, 0, 1 },\n");
    fprintf(fp, "%s", end);
    fclose(fp);
    free(old);
    free(text);
}

int main(int n, char *args[n]) {
    fillCatTable("UnicodeData.txt", fullTable);
    int dataSize = pack(fullTable, indexTable, dataTable);
//    printf("cats %d %d\n", MAX/256, dataSize);
    print("../unicode.c", "categoryIndex", MAX/256, indexTable);
    print("../unicode.c", "categoryTable", dataSize, dataTable);
    fillGraphTable("GraphemeBreakProperty.txt", fullTable);
    overrideGraphTable("emoji-data.txt", fullTable);
    dataSize = pack(fullTable, indexTable, dataTable);
//    printf("graphs %d %d\n", MAX/256, dataSize);
    print("../unicode.c", "graphemeIndex", MAX/256, indexTable);
    print("../unicode.c", "graphemeTable", dataSize, dataTable);
    fillTests("../unicode.c", "GraphemeBreakTest.txt");
}
