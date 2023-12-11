// Snipe editor. Free and open source, see licence.txt.
#include "types.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

char *typeNames[64] = {
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

char *typeName(Type t) {
    return typeNames[t];
}

Type displayType(Type t) {
    if ((t & Bad) != 0) return Error;
    if (t < FirstB) return t;
    return typeNames[t][0] - 'A' + 2;
}

char visualType(Type t) {
    bool bad = (t & Bad) != 0;
    t = t & ~Bad;
    if (t == None) return '-';
    if (t == Gap) return ' ';
    t = typeNames[t][0];
    if (bad) t = tolower(t);
    return t;
}

bool isBracket(Type t) {
    return FirstB <= t && t <= LastE;
}

bool isOpener(Type t) {
    return FirstB <= t && t <= LastB;
}

bool isCloser(Type t) {
    return FirstE <= t && t <= LastE;
}

bool bracketMatch(Type opener, Type closer) {
    return closer == opener + FirstE - FirstB;
}

bool isPrefix(Type t) {
    t = t & ~Bad;
    switch (t) {
    case BlockB: case Block2B: case BlockE: case Block2E:
    case CommentB: case Comment: case CommentE: case Comment2B: case Comment2E:
    case GroupB: case Group2B: case QuoteB: case Quote2B: case Quote: case Mark:
    case Note: case Operator: case RoundB: case Round2B:
    case SquareB: case Square2B: case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

bool isPostfix(Type t) {
    t = t & ~Bad;
    switch (t) {
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
    for (Type t = 0; t < LastE; t++) {
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
            assert(displayType(t) <= Z && displayType(t|Bad) <= Z);
            assert('A' <= visualType(t) && visualType(t) <= 'Z');
            assert('a' <= visualType(t|Bad) && visualType(t|Bad) <= 'z');
        }
    }
    printf("Types module OK\n");
}

#endif
