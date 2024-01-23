// The Snipe editor is free and open source. See licence.txt.
#include "styles.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

char *styleNames[64] = {
    [Alternative]="Alternative", [Bracket]="Bracket", [Comment]="Comment",
    [Declaration]="Declaration", [Error]="Error", [Function]="Function",
    [Gap]="Gap", [H]="H", [Identifier]="Identifier", [J]="J",
    [Keyword]="Keyword", [L]="L", [Mark]="Mark", [N]="N",
    [Operator]="Operator", [Property]="Property", [Quote]="Quote",
    [R]="R", [Sign]="Sign", [Tag]="Tag", [Unary]="Unary",
    [Value]="Value", [W]="W", [X]="X", [Y]="Y",[Z]="Z",
    [Ground]="Ground", [Select]="Select", [Focus]="Focus",
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

char *styleName(int s) {
    return styleNames[s];
}

int findStyle(char const *name) {
    for (int style = 0; style < LastE; style++) {
        char *s = styleNames[style];
        if (strcmp(name, s) == 0) return style;
        if (style > Caret) continue;
        if (strncmp(name, s, strlen(name)) == 0) return style;
    }
    return -1;
}

char visualStyle(int style) {
    bool bad = (style & Bad) != 0;
    bool first = (style & First) != 0;
    if (! first) return '-';
    style = (style & ~Flags);
    if (style == Gap) return ' ';
    style = styleNames[style][0];
    if (bad) style = tolower(style);
    return style;
}

bool isBracket(int style) {
    style = style & ~Flags;
    return FirstB <= style && style <= LastE;
}

bool isOpener(int style) {
    style = style & ~Flags;
    return FirstB <= style && style <= LastB;
}

bool isCloser(int style) {
    style = style & ~Flags;
    return FirstE <= style && style <= LastE;
}

bool bracketMatch(int opener, int closer) {
    opener = opener & ~Flags;
    closer = closer & ~Flags;
    return closer == opener + FirstE - FirstB;
}

bool isPrefix(int style) {
    style = style & ~Flags;
    switch (style) {
    case BlockB: case Block2B: case BlockE: case Block2E:
    case CommentB: case Comment: case CommentE: case Comment2B: case Comment2E:
    case GroupB: case Group2B: case QuoteB: case Quote2B: case Quote: case Sign:
    case Operator: case RoundB: case Round2B:
    case SquareB: case Square2B: case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

bool isPostfix(int style) {
    style = style & ~Flags;
    switch (style) {
    case BlockB: case Block2B: case GroupE: case Group2E: case Sign:
    case Operator: case RoundE: case Round2E: case SquareE: case Square2E:
    case TagB: case Tag: case TagE:
        return true;
    default: return false;
    }
}

// Assume Ground, Select, Focus, Warn are contiguous.
int addBackground(int s, int bg) {
    assert(Ground <= bg && bg <= Warn);
    if ((s & 0x60) == 0) s = s + ((bg - Ground) << 5);
    return s;
}

int displayStyle(int s) {
    bool bad = (s & Bad) != 0;
    s = s & ~Flags;
    if (s >= FirstB) switch (s) {
    case QuoteB: case Quote2B: case QuoteE: case Quote2E: s = Quote; break;
    case RoundB: case Round2B: case RoundE: case Round2E: s = Bracket; break;
    case SquareB: case Square2B: case SquareE: case Square2E: s = Bracket; break;
    case GroupB: case Group2B: case GroupE: case Group2E: s = Bracket; break;
    case BlockB: case Block2B: case BlockE: case Block2E: s = Bracket; break;
    case TagB: case TagE: s = Tag; break;
    case CommentB: case Comment2B:
    case CommentE: case Comment2E: s = Comment; break;
    }
    if (bad || s == Error) s = addBackground(s, Warn);
    return s;
}

// Add a caret flag to a style.
int addCaret(int s) {
    return s | 0x80;
}

// Get the foreground, background, or caret flag from a style
int foreground(int s) { return s & 0x1F; }
int background(int s) { return Ground + ((s >> 5) & 0x03); }
bool hasCaret(int s) { return (s & 0x80) != 0; }

// ---------- Testing ----------------------------------------------------------
#ifdef stylesTest

int main() {
    for (int k = 0; k < LastE; k++) {
        assert(styleNames[k] != NULL);
        assert(visualStyle(k) == '-');
        int kf = k | First;
        if (k == Gap) {
            assert(displayStyle(kf) == k);
            assert(visualStyle(kf) == ' ');
        }
        else if (k == Error) {
            assert(displayStyle(kf) == addBackground(Error,Warn));
            assert(visualStyle(kf) == 'E');
        }
        else if (k <= Z) {
            assert(displayStyle(kf) <= Z);
            assert('A' <= visualStyle(kf) && visualStyle(kf) <= 'Z');
            assert('a' <= visualStyle(kf|Bad) && visualStyle(kf|Bad) <= 'z');
        }
    }
    int style = addBackground(displayStyle(X), Select);
    assert(foreground(style) == X && background(style) == Select);
    style = addBackground(displayStyle(X|Bad), Focus);
    assert(foreground(style) == X && background(style) == Warn);
    style = addCaret(addBackground(displayStyle(X), Select));
    assert(foreground(style) == X && background(style) == Select);
    assert(hasCaret(style));
    assert((Gap|First)==134);
    assert(visualStyle(134) == ' ');
    printf("Styles module OK\n");
}

#endif
