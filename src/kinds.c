// The Snipe editor is free and open source. See licence.txt.
#include "kinds.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

char *kindNames[64] = {
    [Alternative]="Alternative", [Bracket]="Bracket", [Comment]="Comment",
    [Declaration]="Declaration", [Error]="Error", [Function]="Function",
    [Gap]="Gap", [H]="H", [Identifier]="Identifier", [J]="J",
    [Keyword]="Keyword", [L]="L", [Mark]="Mark", [N]="N",
    [Operator]="Operator", [Property]="Property", [Quote]="Quote",
    [R]="R", [Sign]="Sign", [Tag]="Tag", [Unary]="Unary",
    [Value]="Value", [W]="W", [X]="X", [Y]="Y",[Z]="Z",
    [More]="More", [Ground]="Ground", [Select]="Select", [Focus]="Focus",
    [Warn]="Warn", [Caret]="Caret",

    [QuoteB]="QuoteB", [Quote2B]="Quote2B",
    [CommentB]="CommentB", [Comment2B]="Comment2B", [TagB]="TagB",
    [RoundB]="RoundB", [Round2B]="Round2B", [SquareB]="SquareB",
    [Square2B]="Square2B", [GroupB]="GroupB", [Group2B]="Group2B",
    [BlockB]="BlockB", [Block2B]="Block2B",

    [QuoteE]="QuoteE", [Quote2E]="Quote2E",
    [CommentE]="CommentE", [Comment2E]="Comment2E", [TagE]="TagE",
    [RoundE]="RoundE", [Round2E]="Round2E", [SquareE]="SquareE",
    [Square2E]="Square2E", [GroupE]="GroupE", [Group2E]="Group2E",
    [BlockE]="BlockE", [Block2E]="Block2E"
};

char *kindName(int k) {
    return kindNames[k];
}

int findKind(char const *name) {
    for (int k = 0; k < LastE; k++) {
        char *s = kindNames[k];
        if (strcmp(name, s) == 0) return k;
        if (k > Caret) continue;
        if (strncmp(name, s, strlen(name)) == 0) return k;
    }
    return -1;
}

char visualKind(int k) {
    bool bad = (k & Bad) != 0;
    k = k & ~Bad;
    if (k == More) return '-';
    if (k == Gap) return ' ';
    k = kindNames[k][0];
    if (bad) k = tolower(k);
    return k;
}

bool isBracket(int k) {
    k = k & ~Bad;
    return FirstB <= k && k <= LastE;
}

bool isOpener(int k) {
    k = k & ~Bad;
    return FirstB <= k && k <= LastB;
}

bool isCloser(int k) {
    k = k & ~Bad;
    return FirstE <= k && k <= LastE;
}

bool bracketMatch(int opener, int closer) {
    opener = opener & ~Bad;
    closer = closer & ~Bad;
    return closer == opener + FirstE - FirstB;
}

bool isPrefix(int k) {
    k = k & ~Bad;
    switch (k) {
    case BlockB: case Block2B: case BlockE: case Block2E:
    case CommentB: case Comment: case CommentE: case Comment2B: case Comment2E:
    case GroupB: case Group2B: case QuoteB: case Quote2B: case Quote: case Sign:
    case Operator: case RoundB: case Round2B:
    case SquareB: case Square2B: case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

bool isPostfix(int k) {
    k = k & ~Bad;
    switch (k) {
    case BlockB: case Block2B: case GroupE: case Group2E: case Sign:
    case Operator: case RoundE: case Round2E: case SquareE: case Square2E:
    case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

// Assume Ground, Select, Focus, Warn are contiguous.
byte addBackground(byte s, int k) {
    assert(Ground <= k && k <= Warn);
    if ((s & 0x60) == 0) s = s + ((k - Ground) << 5);
    return s;
}

byte toStyle(int k) {
    bool bad = (k & Bad) != 0;
    k = k & ~Bad;
    if (k >= FirstB) switch (k) {
    case QuoteB: case Quote2B: case QuoteE: case Quote2E: k = Quote; break;
    case RoundB: case Round2B: case RoundE: case Round2E: k = Bracket; break;
    case SquareB: case Square2B: case SquareE: case Square2E: k = Bracket; break;
    case GroupB: case Group2B: case GroupE: case Group2E: k = Bracket; break;
    case BlockB: case Block2B: case BlockE: case Block2E: k = Bracket; break;
    case TagB: case TagE: k = Tag; break;
    case CommentB: case Comment2B:
    case CommentE: case Comment2E: k = Comment; break;
    }
    if (bad || k == Error) k = addBackground(k, Warn);
    return k;
}


// Add a caret flag to a style.
byte addCaret(byte s) {
    return s | 0x80;
}

// Get the foreground, background, or caret flag from a style
int foreground(byte s) { return s & 0x1F; }
int background(byte s) { return Ground + ((s >> 5) & 0x03); }
bool hasCaret(byte s) { return (s & 0x80) != 0; }

// ---------- Testing ----------------------------------------------------------
#ifdef kindsTest

int main() {
    for (int k = 0; k < LastE; k++) {
        assert(kindNames[k] != NULL);
        if (k == More) {
            assert(toStyle(k) == k);
            assert(visualKind(k) == '-');
        }
        else if (k == Gap) {
            assert(toStyle(k) == k);
            assert(visualKind(k) == ' ');
        }
        else {
            assert(toStyle(k) <= Caret || (toStyle(k) & 0x1F) == Error);
            assert('A' <= visualKind(k) && visualKind(k) <= 'Z');
            assert('a' <= visualKind(k|Bad) && visualKind(k|Bad) <= 'z');
        }
    }
    int style = addBackground(toStyle(X), Select);
    assert(foreground(style) == X && background(style) == Select);
    style = addBackground(toStyle(X|Bad), Focus);
    assert(foreground(style) == X && background(style) == Warn);
    style = addCaret(addBackground(toStyle(X), Select));
    assert(foreground(style) == X && background(style) == Select);
    assert(hasCaret(style));
    printf("Kinds module OK\n");
}

#endif
