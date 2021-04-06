// The Snipe editor is free and open source, see licence.txt.
#include "style.h"
#include "string.h"
#include "ctype.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static char *styleNames[] = {
    [CURSOR] = "CURSOR", [SELECT]="SELECT", [BYTE]="BYTE", [POINT]="POINT",
    [GRAPH]="GRAPH", [GAP]="GAP", [WORD]="WORD", [NAME]="NAME", [ID]="ID",
    [VARIABLE]="VARIABLE", [FIELD]="FIELD", [FUNCTION]="FUNCTION", [KEY]="KEY",
    [RESERVED]="RESERVED", [PROPERTY]="PROPERTY", [TYPE]="TYPE", [SIGN]="SIGN",
    [LABEL]="LABEL", [OP]="OP", [NUMBER]="NUMBER", [STRING]="STRING",
    [CHAR]="CHAR", [COMMENT]="COMMENT", [NOTE]="NOTE", [BAD]="BAD",
};

static style styleDefaults[] = {
    [NAME]=WORD, [ID]=WORD, [VARIABLE]=WORD, [FIELD]=WORD, [FUNCTION]=WORD,
    [RESERVED]=KEY, [PROPERTY]=KEY, [TYPE]=KEY, [LABEL]=SIGN, [OP]=SIGN,
    [CHAR]=STRING, [NOTE]=COMMENT,
};

static char styleLetters[] = {
    [CURSOR] = 'c', [SELECT]='s', [BYTE]='b', [POINT]='p', [GRAPH]='g',
    [GAP]='G', [WORD]='W', [NAME]='M', [ID]='I', [VARIABLE]='V', [FIELD]='D',
    [FUNCTION]='F', [KEY]='K', [RESERVED]='R', [PROPERTY]='P', [TYPE]='T',
    [SIGN]='X', [LABEL]='L', [OP]='O', [NUMBER]='N', [STRING]='S', [CHAR]='C',
    [COMMENT]='Z', [NOTE]='Y', [BAD]='B',
};

static void error(char *s, char *n) {
    fprintf(stderr, "Error: %s %s\n", s, n);
    exit(1);
}

style findStyle(char *name) {
    int result = -1;
    for (style s = 0; s < COUNT_STYLES; s++) {
        if (strncmp(name, styleNames[s], strlen(name)) == 0) {
            if (result != -1) error("Duplicate style name", name);
            result = s;
        }
    }
    if (result == -1) error("Unknown style name", name);
    return result;
}

style styleDefault(style s) {
    style d = styleDefaults[s];
    if (d == 0) d = s;
    return d;
}

char *styleName(style s) { return styleNames[s]; }

char styleLetter(style s) {
    if ((s & 0x80) != 0) return styleLetters[BAD];
    return styleLetters[s];
}

style badStyle(style s) {
    if (s < BAD) s += BAD;
    return s;
}

style goodStyle(style s) {
    if (s >= BAD) s = s - BAD;
    return s;
}

bool isBadStyle(style s) {
    return s >= BAD;
}

#ifdef styleTest

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    assert(findStyle("CURSOR") == CURSOR);
    assert(strcmp(styleName(CURSOR), "CURSOR") == 0);
    assert(findStyle("SELECT") == SELECT);
    assert(strcmp(styleName(SELECT), "SELECT") == 0);
    assert(findStyle("BAD") == BAD);
    assert(strcmp(styleName(BAD), "BAD") == 0);
    assert(badStyle(GAP) == BAD + GAP);
    assert(isBadStyle(BAD + GAP) == true);
    assert(isBadStyle(GAP) == false);
    assert(goodStyle(BAD + GAP) == GAP);
    assert(goodStyle(GAP) == GAP);
    printf("Style module OK\n");
    return 0;
}

#endif
