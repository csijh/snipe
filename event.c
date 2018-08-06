// The Snipe editor is free and open source, see licence.txt.
#include "event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Event names, excluding prefixes.
static char *eventNames[] = {
    [WORLD_1]="WORLD_1", [WORLD_2]="WORLD_2", [ESCAPE]="ESCAPE",
    [ENTER]="ENTER", [TAB]="TAB", [BACKSPACE]="BACKSPACE", [INSERT]="INSERT",
    [DELETE]="DELETE", [RIGHT]="RIGHT", [LEFT]="LEFT", [DOWN]="DOWN", [UP]="UP",
    [PAGE_UP]="PAGE_UP", [PAGE_DOWN]="PAGE_DOWN", [HOME]="HOME", [END]="END",
    [F1]="F1", [F2]="F2", [F3]="F3", [F4]="F4", [F5]="F5", [F6]="F6", [F7]="F7",
    [F8]="F8", [F9]="F9", [F10]="F10", [F11]="F11", [F12]="F12", [F13]="F13",
    [F14]="F14", [F15]="F15", [F16]="F16", [F17]="F17", [F18]="F18",
    [F19]="F19", [F20]="F20", [F21]="F21", [F22]="F22", [F23]="F23",
    [F24]="F24", [F25]="F25", [KP_0]="KP_0", [KP_1]="KP_1", [KP_2]="KP_2",
    [KP_3]="KP_3", [KP_4]="KP_4", [KP_5]="KP_5", [KP_6]="KP_6", [KP_7]="KP_7",
    [KP_8]="KP_8", [KP_9]="KP_9", [KP_DECIMAL]="KP_DECIMAL",
    [KP_DIVIDE]="KP_DIVIDE", [KP_MULTIPLY]="KP_MULTIPLY",
    [KP_SUBTRACT]="KP_SUBTRACT", [KP_ADD]="KP_ADD", [KP_ENTER]="KP_ENTER",
    [KP_EQUAL]="KP_EQUAL", [MENU]="MENU", [CLICK]="CLICK", [DRAG]="DRAG",
    [LINE_UP]="LINE_UP", [LINE_DOWN]="LINE_DOWN", [TEXT]="TEXT",
    [REDRAW]="REDRAW", [TICK]="TICK", [LOAD]="LOAD", [BLINK]="BLINK",
    [SAVE]="SAVE", [QUIT]="QUIT"
};

event addEventFlag(event flag, event e) {
    return flag | e;
}

bool hasEventFlag(event flag, event e) {
    return (SC_ & e) == flag;
}

event clearEventFlags(event e) {
    return e & ~SC_;
}

const char *findEventName(event e) { return eventNames[e]; }

event findEvent(char *name) {
    event flag = 0;
    if (strncmp(name, "S_", 2) == 0) {
        flag = S_;
        name = name + 2;
    }
    else if (strncmp(name, "C_", 2) == 0) {
        flag = C_;
        name = name + 2;
    }
    else if (strncmp(name, "SC_", 3) == 0) {
        flag = SC_;
        name = name + 3;
    }
    for (event e = 0; e <= QUIT; e++) {
        if (strcmp(eventNames[e], name) == 0) return addEventFlag(flag, e);
    }
    printf("Unknown event name %s\n", name);
    exit(1);
}

void printEvent(event e, int r, int c, char *t) {
    if (e == addEventFlag(C_, TEXT)) {
        printf("C_%s", t);
        return;
    }
    if (hasEventFlag(SC_, e)) printf("SC_");
    else if (hasEventFlag(S_, e)) printf("S_");
    else if (hasEventFlag(C_, e)) printf("C_");
    e = clearEventFlags(e);
    if (e == TEXT) {
        printf("TEXT %s", t);
    } else if (e == CLICK || e == DRAG) {
        printf("%s r=%d c=%d", findEventName(e), r, c);
    } else {
        printf("%s", findEventName(e));
    }
}

#ifdef test_event

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    assert(hasEventFlag(S_, TAB) == false);
    assert(hasEventFlag(SC_, addEventFlag(SC_, TAB)) == true);
    assert(hasEventFlag(S_, addEventFlag(SC_, TAB)) == false);
    assert(strcmp(findEventName(TAB), "TAB") == 0);
    assert(findEvent("TAB") == TAB);
    assert(findEvent("S_TAB") == addEventFlag(S_, TAB));
    printf("Event module OK\n");
    return 0;
}

#endif
