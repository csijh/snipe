// Snipe editor. Free and open source. See licence.txt.
// Coordinate the available languages.
#include "lang.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int style(int tag) {
    switch (tag) {
        case PreSign: case InSign: case PostSign: return Sign;
        case PreOp: case InOp: case PostOp: return Op;
        case Open: case Close: case Open1: case Close1: return Sign;
        case Open2: case Close2: case Begin: case End: return Sign;
        case OpenC: case BeginC: case CloseC: case EndC: return Sign;
        case Quote: case Misquote: case Quotes: case Misquotes:
        case StartQ: case StopQ: return Quoted;
        case Note: case StartC: case StopC: return Commented;
        default: return tag;
    }
}

bool postfix(int tag) {
    switch (tag) {
        case PreSign: case InSign: return true;
        case PreOp: case InOp: return true;
        case Open: case Open1: return true;
        case Open2: case Begin: case End: return true;
        case OpenC: case BeginC: case EndC: return true;
        default: return false;
    }
}

bool prefix(int tag) {
    switch (tag) {
        case InSign: case PostSign: return true;
        case InOp: case PostOp: return true;
        case Close: case Close1: return true;
        case Close2: case Begin: case End: return true;
        case BeginC: case CloseC: case EndC: return true;
        default: return false;
    }
}

static char *nicknames[nTags] = {
    [Gap]=" ", [Mark]="M", [Warn]="W", [Here]="H", [Newline]="\n", [Bad]="B",
    [Commented]="C", [Quoted]="Q", [Escape]="\\",
    [Sign]="S", [Op]="O", [Value]="V", [Key]="K", [Type]="T",
    [Reserved]="R", [Id]="I", [Property]="P", [Function]="F",
    [PreSign]="x", [InSign]=":", [PostSign]="y",
    [PreOp]="r", [InOp]="o", [PostOp]="l",
    [Open]="(", [Close]=")", [Open1]="[", [Close1]="]",
    [Open2]="{", [Close2]="}", [Begin]="<", [End]=">",
    [OpenC]="{", [CloseC]="}", [BeginC]="<", [EndC]=">",
    [Quote]="'", [Misquote]="?", [Quotes]="\"", [Misquotes]="?",
    [Note]="#", [StartQ]="\"", [StopQ]="\"", [StartC]="%", [StopC]="^"
};

char *nickname(int tag) {
    return nicknames[tag];
}

void showToken(token t, char *alt, char *out) {
    out = out + strlen(out);
    if (alt != NULL && strlen(alt) > 0) {
        int n = strlen(alt);
        sprintf(out, "%s", alt);
        out = out + n;
        for (int i = n; i < t.length; i++) sprintf(out++, "%c", '~');
        return;
    }
    char *s = nickname(t.tag);
    sprintf(out++, "%c", s[0]);
    if (s[0] == ' ') {
        for (int i = 1; i < t.length; i++) sprintf(out++, "%c", ' ');
    }
    else {
        for (int i = 1; i < t.length; i++) sprintf(out++, "%c", '~');
    }
}

#ifdef TEST

// It may be useful for tags to leave two bits spare.
static void testTags() {
    assert(nTags <= 64);
    for (int tag = 0; tag < nTags; tag++) {
        char *name = nicknames[tag];
        assert(name != NULL && strlen(name) == 1);
    }
}

void testLang() {
    testTags();
    printf("lang module OK\n");
}

#endif

#ifdef TESTlang

int main() {
    testLang();
    return 0;
}

#endif
