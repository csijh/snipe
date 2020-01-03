// The Snipe editor is free and open source, see licence.txt.

// Action constants. Events are mapped to these, which are then mapped to edits.
// The actions which are appied to each cursor when there are multiple cursors
// are listed first.
enum action {
    MoveLeftChar, MoveRightChar, MoveLeftWord, MoveRightWord, MoveUpLine,
    MoveDownLine, MoveStartLine, MoveEndLine, MarkLeftChar, MarkRightChar,
    MarkLeftWord, MarkRightWord, MarkUpLine, MarkDownLine, MarkStartLine,
    MarkEndLine, CutLeftChar, CutRightChar, CutLeftWord, CutRightWord,
    CutUpLine, CutDownLine, CutStartLine, CutEndLine, Newline, Insert, Cut,
    Copy, Paste,

    Point, Select, AddPoint, AddSelect,  Undo, Redo, Load, Save, Open,
    Bigger, Smaller, CycleTheme, PageUp, PageDown, Resize, Focus, Defocus,
    Blink, Frame, Scroll, Help, Quit, Ignore,
    COUNT_ACTIONS = Ignore + 1
};
typedef int action;

// Find an action from its name.
action findAction(char *name);

// Find the name of an action.
char *findActionName(action a);

// Print out an action, e.g. for testing.
void printAction(action a);

// TODO:  Move/Mark/Cut  Back/Fore  Char/Word/Line/Row
// OR:  Multi-cursor, collapse-cursor, irrelevant.

// MoveLeftChar -> find cursor, then either moveBase or moveMark or moveCursor.
