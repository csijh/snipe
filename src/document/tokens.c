// Token and grapheme scanner. Free and open source. See licence.txt.
#include "tokens.h"
#include <stdio.h>
#include <assert.h>

// Characters which visualize tags.
static char tagChar[] = {
    [Byte]='.', [Grapheme]=' ', [Gap]='_', [Operator]='+', [Label]=':',
    [Quote]='\'', [Quotes]='"', [OpenR]='(', [CloseR]=')', [OpenS]='[',
    [CloseS]=']', [OpenC]='{', [CloseC]='}', [OpenI]='%', [CloseI]='|',
    [Comment]='<', [EndComment]='>', [Note]='/', [Newline]='\n', [Invalid]='?',
    [AToken]='A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

char showTag(int tag) {
    switch (tag & 0xC0) {
        case Commented: return tagChar[Gap];
        case Quoted: return tagChar[Gap];
        case Unmatched: return tagChar[Invalid];
        default: return tagChar[tag & 0x3F];
    }
}

#ifdef tokensTest

int main(int n, char const *args[n]) {
    assert(showTag(CloseR) == ')');
    assert(showTag(AToken + ('X' - 'A')) == 'X');
    assert(showTag(CloseR | Commented) == '_');
    assert(showTag(CloseR | Unmatched) == '?');
    printf("Tokens module OK\n");
    return 0;
}

#endif
