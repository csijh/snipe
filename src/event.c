// The Snipe editor is free and open source, see licence.txt.
#include "event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Event names, with text keys and prefixes added later.
static char *eventNames[COUNT_EVENTS] = {
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
    [PASTE]="PASTE", [RESIZE]="RESIZE", [TICK]="TICK", [LOAD]="LOAD",
    [BLINK]="BLINK", [SAVE]="SAVE", [QUIT]="QUIT"
};

// Space for generated strings.
enum { MAX = 16 };
static char spare[COUNT_EVENTS][MAX];

// Fill in the names for valid text keys and prefix combinations.
static void fillTable() {
    eventNames[C_ + ' '] = "C_";
    for (int c='!'; c < '~'; c++) {
        spare[C_ + c][0] = 'C';
        spare[C_ + c][1] = '_';
        spare[C_ + c][2] = c;
        spare[C_ + c][3] = '\0';
        eventNames[C_ + c] = spare[C_ + c];
    }
    for (int i = WORLD_1; i < TEXT; i++) {
        if (eventNames[i] == NULL) continue;
        assert(strlen(eventNames[i]) < MAX - 3);
        strcpy(spare[S_ + i], "S_");
        strcat(spare[S_ + i], eventNames[i]);
        eventNames[S_ + i] = spare[S_ + i];
        strcpy(spare[C_ + i], "C_");
        strcat(spare[C_ + i], eventNames[i]);
        eventNames[C_ + i] = spare[C_ + i];
        strcpy(spare[SC_ + i], "SC_");
        strcat(spare[SC_ + i], eventNames[i]);
        eventNames[SC_ + i] = spare[SC_ + i];
    }
}

event addEventFlag(event flag, event e) {
    return flag | e;
}

bool hasEventFlag(event flag, event e) {
    return (SC_ & e) == flag;
}

event clearEventFlags(event e) {
    return e & ~SC_;
}

const char *findEventName(event e) {
    if (eventNames['!'] == NULL) fillTable();
    return eventNames[e];
}

event findEvent(char *name) {
    if (eventNames['!'] == NULL) fillTable();
    for (event e = 0; e <= COUNT_EVENTS; e++) {
        if (eventNames[e] == NULL) continue;
        if (strcmp(eventNames[e], name) == 0) return e;
    }
    printf("Unknown event name %s\n", name);
    exit(1);
}

void printEvent(event e, int r, int c, char const *t) {
    printf("%s", findEventName(e));
    if (e == TEXT) printf(" %s", t);
    else if (e == CLICK || e == DRAG) printf(" r=%d c=%d", r, c);
}

#ifdef test_event

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    assert(hasEventFlag(S_, TAB) == false);
    assert(hasEventFlag(SC_, addEventFlag(SC_, TAB)) == true);
    assert(hasEventFlag(S_, addEventFlag(SC_, TAB)) == false);
    assert(findEvent("TAB") == TAB);
    assert(findEvent("S_TAB") == addEventFlag(S_, TAB));
    assert(findEvent("C_TAB") == addEventFlag(C_, TAB));
    assert(findEvent("SC_TAB") == addEventFlag(SC_, TAB));
    assert(strcmp(findEventName(TAB), "TAB") == 0);
    assert(strcmp(findEventName(addEventFlag(S_, TAB)), "S_TAB") == 0);
    assert(strcmp(findEventName(addEventFlag(C_, TAB)), "C_TAB") == 0);
    assert(strcmp(findEventName(addEventFlag(SC_, TAB)), "SC_TAB") == 0);
    assert(findEvent("C_+") == addEventFlag(C_, '+'));
    assert(strcmp(findEventName(addEventFlag(C_, '+')), "C_+") == 0);
    printf("Event module OK\n");
    return 0;
}

#endif
