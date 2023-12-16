// Snipe editor. Free and open source, see licence.txt.
#include "kinds.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

char *kindNames[64] = {
    [None]="None", [Gap]="Gap",
    [Alternative]="Alternative", [Block]="Block", [Comment]="Comment",
    [Declaration]="Declaration", [Error]="Error", [Function]="Function",
    [Group]="Group", [H]="H", [Identifier]="Identifier", [Jot]="Jot",
    [Keyword]="Keyword", [L]="L", [Mark]="Mark", [Note]="Note",
    [Operator]="Operator", [Property]="Property", [Quote]="Quote",
    [Round]="Round", [Square]="Square", [Tag]="Tag", [Unary]="Unary",
    [Value]="Value", [W]="W", [X]="X", [Y]="Y",[Z]="Z",

    [QuoteB]="QuoteB", [Quote2B]="Quote2B", [CommentB]="CommentB",
    [Comment2B]="Comment2B", [TagB]="TagB", [RoundB]="RoundB",
    [Round2B]="Round2B", [SquareB]="SquareB",[Square2B]="Square2B",
    [GroupB]="GroupB", [Group2B]="Group2B", [BlockB]="BlockB",
    [Block2B]="Block2B",

    [QuoteE]="QuoteE", [Quote2E]="Quote2E", [CommentE]="CommentE",
    [Comment2E]="Comment2E", [TagE]="TagE", [RoundE]="RoundE",
    [Round2E]="Round2E", [SquareE]="SquareE",[Square2E]="Square2E",
    [GroupE]="GroupE", [Group2E]="Group2E", [BlockE]="BlockE",
    [Block2E]="Block2E"
};

char *kindName(Kind k) {
    return kindNames[k];
}

Kind displayKind(Kind k) {
    if ((k & Bad) != 0) return Error;
    if (k < FirstB) return k;
    return kindNames[k][0] - 'A' + 2;
}

char visualKind(Kind k) {
    bool bad = (k & Bad) != 0;
    k = k & ~Bad;
    if (k == None) return '-';
    if (k == Gap) return ' ';
    k = kindNames[k][0];
    if (bad) k = tolower(k);
    return k;
}

bool isBracket(Kind k) {
    k = k & ~Bad;
    return FirstB <= k && k <= LastE;
}

bool isOpener(Kind k) {
    k = k & ~Bad;
    return FirstB <= k && k <= LastB;
}

bool isCloser(Kind k) {
    k = k & ~Bad;
    return FirstE <= k && k <= LastE;
}

bool bracketMatch(Kind opener, Kind closer) {
    opener = opener & ~Bad;
    closer = closer & ~Bad;
    return closer == opener + FirstE - FirstB;
}

bool isPrefix(Kind k) {
    k = k & ~Bad;
    switch (k) {
    case BlockB: case Block2B: case BlockE: case Block2E:
    case CommentB: case Comment: case CommentE: case Comment2B: case Comment2E:
    case GroupB: case Group2B: case QuoteB: case Quote2B: case Quote: case Mark:
    case Note: case Operator: case RoundB: case Round2B:
    case SquareB: case Square2B: case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

bool isPostfix(Kind k) {
    k = k & ~Bad;
    switch (k) {
    case BlockB: case Block2B: case GroupE: case Group2E: case Mark:
    case Operator: case RoundE: case Round2E: case SquareE: case Square2E:
    case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

// ---------- Testing ----------------------------------------------------------
#ifdef kindsTest

int main() {
    for (Kind k = 0; k < LastE; k++) {
        assert(kindNames[k] != NULL);
        if (k == None) {
            assert(displayKind(k) == k);
            assert(visualKind(k) == '-');
        }
        else if (k == Gap) {
            assert(displayKind(k) == k);
            assert(visualKind(k) == ' ');
        }
        else {
            assert(displayKind(k) <= Z && displayKind(k|Bad) <= Z);
            assert('A' <= visualKind(k) && visualKind(k) <= 'Z');
            assert('a' <= visualKind(k|Bad) && visualKind(k|Bad) <= 'z');
        }
    }
    printf("Kinds module OK\n");
}

#endif
