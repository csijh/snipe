// The Snipe editor is free and open source, see licence.txt.
#include "style.h"
#include "file.h"
#include "string.h"
#include "ctype.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static char *styleNames[] = {
    [START] = "START", [POINT]="POINT", [SELECT]="SELECT", [GAP]="GAP",
    [WORD]="WORD", [NAME]="NAME", [ID]="ID", [VARIABLE]="VARIABLE",
    [FIELD]="FIELD", [FUNCTION]="FUNCTION", [KEY]="KEY", [RESERVED]="RESERVED",
    [PROPERTY]="PROPERTY", [TYPE]="TYPE", [SIGN]="SIGN", [LABEL]="LABEL",
    [OP]="OP", [NUMBER]="NUMBER", [STRING]="STRING", [CHAR]="CHAR",
    [COMMENT]="COMMENT", [NOTE]="NOTE", [BAD]="BAD",
};

static style styleDefaults[] = {
    [NAME]=WORD, [ID]=WORD, [VARIABLE]=WORD, [FIELD]=WORD, [FUNCTION]=WORD,
    [RESERVED]=KEY, [PROPERTY]=KEY, [TYPE]=KEY, [LABEL]=SIGN, [OP]=SIGN,
    [CHAR]=STRING, [NOTE]=COMMENT,
};

static char styleLetters[] = {
    [START] = '?', [POINT]='?', [SELECT]='?', [GAP]='G',
    [WORD]='W', [NAME]='M', [ID]='I', [VARIABLE]='V',
    [FIELD]='D', [FUNCTION]='F', [KEY]='K', [RESERVED]='R',
    [PROPERTY]='P', [TYPE]='T', [SIGN]='X', [LABEL]='L',
    [OP]='O', [NUMBER]='N', [STRING]='S', [CHAR]='C',
    [COMMENT]='Z', [NOTE]='Y', [BAD]='B',
};

static void error(char *s, char *n) {
    printf("Error: %s %s\n", s, n);
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
    char ch = styleLetters[clearStyleFlags(s)];
    if (! hasStyleFlag(s, START)) ch = tolower(ch);
    return ch;
}

static inline bool isFlag(style s) { return s <= SELECT; }

compoundStyle addStyleFlag(style s, style flag) {
    assert(isFlag(flag));
    return s | (0x20 << flag);
}

// Check for a flag.
bool hasStyleFlag(compoundStyle s, style flag) {
    assert(isFlag(flag));
    return (s & (0x20 << flag)) != 0;
}

// Take off any flags.
style clearStyleFlags(compoundStyle s) {
    return s & 0x1F;
}

#ifdef styleTest

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    assert(findStyle("POINT") == POINT);
    assert(strcmp(styleName(POINT), "POINT") == 0);
    assert(findStyle("SELECT") == SELECT);
    assert(strcmp(styleName(START), "START") == 0);
    assert(addStyleFlag(GAP, POINT) == 0x43);
    assert(hasStyleFlag(GAP, POINT) == false);
    assert(hasStyleFlag(addStyleFlag(GAP, POINT), POINT) == true);
    assert(clearStyleFlags(addStyleFlag(GAP, POINT)) == GAP);
    printf("Style module OK\n");
    return 0;
}

#endif
