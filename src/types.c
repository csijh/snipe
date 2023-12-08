// Snipe editor. Free and open source, see licence.txt.
#include "types.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

char *typeNames[64] = {
    [Alternative]="Alternative", [B]="B", [Comment]="Comment",
    [Declaration]="Declaration", [E]="E", [Function]="Function", [G]="G",
    [H]="H", [Identifier]="Identifier", [Jot]="Jot", [Keyword]="Keyword",
    [L]="L", [Mark]="Mark", [Note]="Note", [Operator]="Operator",
    [Property]="Property", [Quote]="Quote", [R]="R", [S]="S", [Tag]="Tag",
    [Unary]="Unary", [Value]="Value", [Wrong]="Wrong", [X]="X", [Y]="Y",
    [Z]="Z", [None]="None", [Gap]="Gap",

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

char *typeName(int type) {
    return typeNames[type];
}

byte displayType(int type) {
    bool bad = (type & Bad) != 0;
    type = type & ~Bad;
    if (type < FirstB) return type;
    type = typeNames[type][0] - 'A';
    if (bad) type = type | Bad;
    return type;
}

char visualType(int type) {
    bool bad = (type & Bad) != 0;
    type = type & ~Bad;
    if (type == None) return '-';
    if (type == Gap) return ' ';
    type = typeNames[type][0];
    if (bad) type = tolower(type);
    return type;
}

bool isBracket(int type) {
    return FirstB <= type && type <= LastE;
}

bool isOpener(int type) {
    return FirstB <= type && type <= LastB;
}

bool isCloser(int type) {
    return FirstE <= type && type <= LastE;
}

bool bracketMatch(int opener, int closer) {
    return closer == opener + FirstE - FirstB;
}

bool isPrefix(int type) {
    type = type & ~Bad;
    switch (type) {
    case BlockB: case Block2B: case BlockE: case Block2E:
    case CommentB: case Comment: case CommentE: case Comment2B: case Comment2E:
    case GroupB: case Group2B: case QuoteB: case Quote2B: case Quote: case Mark:
    case Note: case Operator: case RoundB: case Round2B:
    case SquareB: case Square2B: case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

bool isPostfix(int type) {
    type = type & ~Bad;
    switch (type) {
    case BlockB: case Block2B: case GroupE: case Group2E: case Mark:
    case Operator: case RoundE: case Round2E: case SquareE: case Square2E:
    case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

// ---------- Testing ----------------------------------------------------------
#ifdef typesTest

int main() {
    for (int t = 0; t < LastE; t++) {
        assert(typeNames[t] != NULL);
        if (t == None) {
            assert(displayType(t) == t);
            assert(visualType(t) == '-');
        }
        else if (t == Gap) {
            assert(displayType(t) == t);
            assert(visualType(t) == ' ');
        }
        else {
            assert(displayType(t) < 26);
            assert('A' <= visualType(t) && visualType(t) <= 'Z');
            assert('a' <= visualType(t|Bad) && visualType(t|Bad) <= 'z');
        }
    }
    printf("Types module OK\n");
}

#endif
