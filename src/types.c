// The full names of the types.
char *typeNames[64] = {
    [Alternative]="Alternative", [B]="B", [Comment]="Comment",
    [Declaration]="Declaration", [E]="E", [Function]="Function", [G]="G",
    [H]="H", [Identifier]="Identifier", [Jot]="Jot", [Keyword]="Keyword",
    [Long]="Long", [Mark]="Mark", [Note]="Note", [Operator]="Operator",
    [Property]="Property", [Quote]="Quote", [R]="R", [S]="S", [Tag]="Tag",
    [Unary]="Unary", [Value]="Value", [Wrong]="Wrong", [X]="X", [Y]="Y",
    [Z]="Z", [None]="None", [Gap]="Gap", [Indent]="Indent",

    [NoteD]="NoteD", [QuoteD]="QuoteD",

    [LongB]="LongB", [CommentB]="CommentB", [Comment2B]="Comment2B",
    [TagB]="TagB", [RoundB]="RoundB", [Round2B]="Round2B", [SquareB]="SquareB",
    [Square2B]="Square2B", [GroupB]="GroupB", [Group2B]="Group2B",
    [BlockB]="BlockB", [Block2B]="Block2B",

    [LongE]="LongE", [CommentE]="CommentE", [Comment2E]="Comment2E",
    [TagE]="TagE", [RoundE]="RoundE", [Round2E]="Round2E", [SquareE]="SquareE",
    [Square2E]="Square2E", [GroupE]="GroupE", [Group2E]="Group2E",
    [BlockE]="BlockE", [Block2E]="Block2E"
};

char *typeName(int type) {
    return typeNames[type];
}

char *compactType(int type) {

}


char *letterType(int type) {

}
