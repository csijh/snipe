// Snipe event handling. Free and open source. See licence.txt.
#include "events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Event names, with S_ C_ SC_ combinations added later for the first group.
static char *eventNames[IGNORE+1] = {
    [CLICK]="CLICK", [DRAG]="DRAG", [SCROLL]="SCROLL", [ESCAPE]="ESCAPE",
    [ENTER]="ENTER", [TAB]="TAB", [BACKSPACE]="BACKSPACE", [INSERT]="INSERT",
    [DELETE]="DELETE", [RIGHT]="RIGHT", [LEFT]="LEFT", [DOWN]="DOWN", [UP]="UP",
    [PAGE_UP]="PAGE_UP", [PAGE_DOWN]="PAGE_DOWN", [HOME]="HOME", [END]="END",
    [F1]="F1", [F2]="F2", [F3]="F3", [F4]="F4", [F5]="F5", [F6]="F6", [F7]="F7",
    [F8]="F8", [F9]="F9", [F10]="F10", [F11]="F11", [F12]="F12", [MENU]="MENU",

    [C_A]="C_A", [C_B]="C_B", [C_C]="C_C", [C_D]="C_D", [C_E]="C_E",
    [C_F]="C_F", [C_G]="C_G", [C_H]="C_H", [C_I]="C_I", [C_J]="C_J",
    [C_K]="C_K", [C_L]="C_L", [C_M]="C_M", [C_N]="C_N", [C_O]="C_O",
    [C_P]="C_P", [C_Q]="C_Q", [C_R]="C_R", [C_S]="C_S", [C_T]="C_T",
    [C_U]="C_U", [C_V]="C_V", [C_W]="C_W", [C_X]="C_X", [C_Y]="C_Y",
    [C_Z]="C_Z", [C_0]="C_0", [C_1]="C_1", [C_2]="C_2", [C_3]="C_3",
    [C_4]="C_4", [C_5]="C_5", [C_6]="C_6", [C_7]="C_7", [C_8]="C_8",
    [C_9]="C_9", [C_SPACE]="C_SPACE", [C_PLUS]="C_PLUS", [C_MINUS]="C_MINUS",

    [TEXT]="TEXT", [PASTE]="PASTE", [RESIZE]="RESIZE", [FOCUS]="FOCUS",
    [DEFOCUS]="DEFOCUS", [FRAME]="FRAME", [LOAD]="LOAD", [BLINK]="BLINK",
    [SAVE]="SAVE", [QUIT]="QUIT", [IGNORE]="IGNORE"
};

// Space for generated strings.
enum { MAX = 16 };
static char spare[IGNORE+1][MAX];

// Fill in the names for valid text keys and prefix combinations.
static void fillTable() {
    for (int i = CLICK; i <= MENU; i = i + 4) {
        assert(strlen(eventNames[i]) < MAX - 3);
        strcpy(spare[i+1], "S_");
        strcat(spare[i+1], eventNames[i]);
        eventNames[i+1] = spare[i+1];
        strcpy(spare[i+2], "C_");
        strcat(spare[i+2], eventNames[i]);
        eventNames[i+2] = spare[i+2];
        strcpy(spare[i+3], "SC_");
        strcat(spare[i+3], eventNames[i]);
        eventNames[i+3] = spare[i+3];
    }
}

const char *findEventName(event e) {
    if (eventNames[CLICK+1] == NULL) fillTable();
    if (e > IGNORE) printf("F1 -> %d\n", e);
    return eventNames[e];
}

event findEvent(char *name) {
    if (eventNames[CLICK+1] == NULL) fillTable();
    for (event e = 0; e < IGNORE; e++) {
        if (strcmp(eventNames[e], name) == 0) return e;
    }
    printf("Unknown event name %s\n", name);
    exit(1);
}

void printEvent(event e, int x, int y, char const *t, char *end) {
    printf("%s", findEventName(e));
    if (e == TEXT) printf(" %s", t);
    else if (e == CLICK || e == DRAG || e == SCROLL) {
        printf(" x=%d y=%d", x, y);
    }
    printf("%s", end);
}

#ifdef TESTevents

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    assert(findEvent("TAB") == TAB);
    assert(findEvent("S_TAB") == TAB+1);
    assert(findEvent("C_TAB") == TAB+2);
    assert(findEvent("SC_TAB") == TAB+3);
    assert(findEvent("C_PLUS") == C_PLUS);
    assert(findEvent("QUIT") == QUIT);
    assert(strcmp(findEventName(TAB), "TAB") == 0);
    assert(strcmp(findEventName(S_TAB), "S_TAB") == 0);
    assert(strcmp(findEventName(C_TAB), "C_TAB") == 0);
    assert(strcmp(findEventName(SC_TAB), "SC_TAB") == 0);
    assert(strcmp(findEventName(C_PLUS), "C_PLUS") == 0);
    assert(strcmp(findEventName(QUIT), "QUIT") == 0);
    printf("Event module OK\n");
    return 0;
}

#endif
