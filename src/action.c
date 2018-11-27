// The Snipe editor is free and open source, see licence.txt.
#include "action.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Define the names of the actions.
static char *actionNames[] = {
    [MoveLeftChar]="MoveLeftChar", [MoveRightChar]="MoveRightChar",
    [MoveLeftWord]="MoveLeftWord", [MoveRightWord]="MoveRightWord",
    [MoveUpLine]="MoveUpLine", [MoveDownLine]="MoveDownLine",
    [MoveStartLine]="MoveStartLine", [MoveEndLine]="MoveEndLine",
    [MarkLeftChar]="MarkLeftChar", [MarkRightChar]="MarkRightChar",
    [MarkLeftWord]="MarkLeftWord", [MarkRightWord]="MarkRightWord",
    [MarkUpLine]="MarkUpLine", [MarkDownLine]="MarkDownLine",
    [MarkStartLine]="MarkStartLine", [MarkEndLine]="MarkEndLine",
    [CutLeftChar]="CutLeftChar", [CutRightChar]="CutRightChar",
    [CutLeftWord]="CutLeftWord", [CutRightWord]="CutRightWord",
    [CutUpLine]="CutUpLine", [CutDownLine]="CutDownLine",
    [CutStartLine]="CutStartLine", [CutEndLine]="CutEndLine",
    [Newline]="Newline", [Bigger]="Bigger", [Smaller]="Smaller",
    [CycleTheme]="CycleTheme", [Point]="Point", [Select]="Select",
    [AddPoint]="AddPoint", [AddSelect]="AddSelect", [Insert]="Insert",
    [Cut]="Cut", [Copy]="Copy", [Paste]="Paste", [PageUp]="PageUp",
    [PageDown]="PageDown", [Undo]="Undo", [Redo]="Redo", [Resize]="Resize",
    [Focus]="Focus", [Defocus]="Defocus", [Blink]="Blink", [Frame]="Frame",
    [Scroll]="Scroll", [Load]="Load", [Save]="Save", [Open]="Open",
    [Help]="Help", [Quit]="Quit", [Ignore]="Ignore"
};

// Find an action from its name.
action findAction(char *name) {
    for (action a = 0; a <= COUNT_ACTIONS; a++) {
        if (strcmp(actionNames[a], name) == 0) return a;
    }
    printf("Error: can't find action %s\n", name);
    exit(1);
    return Ignore;
}

char *findActionName(action a) {
    return actionNames[a];
}

void printAction(action a) {
    printf("%s\n", actionNames[a]);
}

#ifdef actionTest

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    assert(findAction("Ignore") == Ignore);
    printf("Action module OK\n");
    return 0;
}

#endif
