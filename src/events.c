// Snipe event handling. Free and open source. See licence.txt.
#include "events.h"
#include <allegro5/allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct events {
    ALLEGRO_DISPLAY *display;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_TIMER *timer;
    char text[8]; int x, y;
    bool mouseButtonDown;
};

static void fail(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

// Check result of a function which must succeed.
static void try(bool b, char *s) { if (!b) fail(s); }

events *newEvents(void *handle) {
    events *es = malloc(sizeof(events));
    es->display = handle;
    es->queue = al_create_event_queue();
    if (es->queue == 0) fail("Failed to create Allegro event queue.");
    try(al_install_keyboard(), "Failed to initialize keyboard.");
    try(al_install_mouse(), "Failed to initialize mouse.");
    try(al_install_touch_input(), "Failed to initialize touchpad.");
    al_register_event_source(es->queue, al_get_display_event_source(handle));
    al_register_event_source(es->queue, al_get_keyboard_event_source());
    al_register_event_source(es->queue, al_get_mouse_event_source());
    es->timer = al_create_timer(2.0);
    if (es->timer == NULL) fail("Failed to create Allegro timer.");
    al_register_event_source(es->queue, al_get_timer_event_source(es->timer));
//    al_set_mouse_emulation_mode(ALLEGRO_MOUSE_EMULATION_TRANSPARENT);
//    al_start_timer(es->timer);
    es->mouseButtonDown = false;
    return es;
}

void freeEvents(events *es) {
    al_destroy_timer(es->timer);
    al_destroy_event_queue(es->queue);
    al_uninstall_touch_input();
    al_uninstall_mouse();
    al_uninstall_keyboard();
    free(es);
}

char *eventText(events *es) { return es->text; }
int eventX(events *es) { return es->x; }
int eventY(events *es) { return es->y; }

// (Copied from unicode.c to make this module self-contained.)
static void putUTF8(unsigned int code, char *s) {
    if (code < 0x7f) {
        s[0] = code;
        s[1] = '\0';
    } else if (code < 0x7ff) {
        s[0] = 0xC0 | (code >> 6);
        s[1] = 0x80 | (code & 0x3F);
        s[2] = '\0';
    } else if (code < 0xffff) {
        s[0] = 0xE0 | (code >> 12);
        s[1] = 0x80 | ((code >> 6) & 0x3F);
        s[2] = 0x80 | (code & 0x3F);
        s[3] = '\0';
    } else if (code <= 0x10FFFF) {
        s[0] = 0xF0 | (code >> 18);
        s[1] = 0x80 | ((code >> 12) & 0x3F);
        s[2] = 0x80 | ((code >> 6) & 0x3F);
        s[3] = 0x80 | (code & 0x3F);
        s[4] = '\0';
    } else {
        s[0] = '\0';
    }
}

// Convert a non-text Allegro keycode into an event. Note that C_SPACE and
// C_0... and keypad keys come here.
static event nonText(events *es, bool shift, bool ctrl, int code) {
    int offset = shift ? (ctrl ? 3 : 1) : (ctrl ? 2 : 0) ;
    if (code >= ALLEGRO_KEY_A && code <= ALLEGRO_KEY_Z) {
        return C_A + (code - ALLEGRO_KEY_A) * 4 + offset;
    }
    if (code >= ALLEGRO_KEY_0 && code <= ALLEGRO_KEY_9) {
        return C_0 + (code - ALLEGRO_KEY_0) * 4;
    }
    if (code >= ALLEGRO_KEY_PAD_0 && code <= ALLEGRO_KEY_PAD_9) {
        if (ctrl) return C_0 + (code - ALLEGRO_KEY_PAD_0);
        else {
            putUTF8('0' + (code - ALLEGRO_KEY_PAD_0), es->text);
            return TEXT;
        }
    }
    if (code >= ALLEGRO_KEY_F1 && code <= ALLEGRO_KEY_F12) {
        return F1 + (code - ALLEGRO_KEY_F1) * 4 + offset;
    }
    if (ctrl && code == ALLEGRO_KEY_PAD_PLUS) return C_PLUS;
    if (ctrl && code == ALLEGRO_KEY_PAD_MINUS) return C_MINUS;
    if (ctrl && code == ALLEGRO_KEY_PAD_SLASH) return IGNORE;
    if (ctrl && code == ALLEGRO_KEY_PAD_ASTERISK) return IGNORE;
    if (ctrl && code == ALLEGRO_KEY_PAD_EQUALS) return IGNORE;
    int ch;
    switch (code) {
        case ALLEGRO_KEY_SPACE: return C_SPACE;
        case ALLEGRO_KEY_ESCAPE: return ESCAPE + offset;
        case ALLEGRO_KEY_BACKSPACE: return BACKSPACE + offset;
        case ALLEGRO_KEY_TAB: return TAB + offset;
        case ALLEGRO_KEY_ENTER: return ENTER + offset;
        case ALLEGRO_KEY_INSERT: return INSERT + offset;
        case ALLEGRO_KEY_DELETE: return DELETE + offset;
        case ALLEGRO_KEY_HOME: return HOME + offset;
        case ALLEGRO_KEY_END: return END + offset;
        case ALLEGRO_KEY_PGUP: return PAGE_UP + offset;
        case ALLEGRO_KEY_PGDN: return PAGE_DOWN + offset;
        case ALLEGRO_KEY_LEFT: return LEFT + offset;
        case ALLEGRO_KEY_RIGHT: return RIGHT + offset;
        case ALLEGRO_KEY_UP: return UP + offset;
        case ALLEGRO_KEY_DOWN: return DOWN + offset;
        case ALLEGRO_KEY_MENU: return MENU + offset;
        case ALLEGRO_KEY_PAD_SLASH: ch = '/'; break;
        case ALLEGRO_KEY_PAD_ASTERISK: ch = '*'; break;
        case ALLEGRO_KEY_PAD_EQUALS: ch = '='; break;
        case ALLEGRO_KEY_PAD_MINUS: ch = '-'; break;
        case ALLEGRO_KEY_PAD_PLUS: ch = '+'; break;
        case ALLEGRO_KEY_PAD_DELETE: return DELETE + offset;
        case ALLEGRO_KEY_PAD_ENTER: return ENTER + offset;
        default: return IGNORE;
    }
    putUTF8(ch, es->text);
    return TEXT;
}

// Translate a keyboard event. Note Allegro (unusually) generates a KEY_CHAR
// event for every keypress, so KEY_DOWN is never needed, and KEY_UP is not
// needed unless actually tracking a long key press. (The 'feature' that
// C_A..C_Z produce unichar 1..26 isn't used, because it mucks up C_ENTER etc.
// and KEYPAD keys must explicitly be sent to nonText.)
static event keyboard(events *es, ALLEGRO_EVENT *ae) {
    int ch = ae->keyboard.unichar;
    int code = ae->keyboard.keycode;
    int mod = ae->keyboard.modifiers;
    if ((mod & ALLEGRO_KEYMOD_ALT) != 0) return IGNORE;
    bool shift = (mod & ALLEGRO_KEYMOD_SHIFT) != 0;
    bool ctrl = (mod & (ALLEGRO_KEYMOD_CTRL|ALLEGRO_KEYMOD_COMMAND)) != 0;
    bool keypad = code >= ALLEGRO_KEY_PAD_0 && code <= ALLEGRO_KEY_PAD_9;
    keypad |= code >= ALLEGRO_KEY_PAD_SLASH && code <= ALLEGRO_KEY_PAD_ENTER;
    keypad |= code == ALLEGRO_KEY_PAD_EQUALS;
    if (' ' <= ch && ch != 127 && ! keypad) {
        if (ctrl) {
            if (ch >= 'A' && ch <= 'Z') return C_A + ch - 'A';
            if (ch >= 'a' && ch <= 'z') return C_A + ch - 'a';
            if (ch >= '0' && ch <= '9') return C_A + ch - '0';
            if (ch == '+' || ch == '=') return C_PLUS;
            if (ch == '-' || ch == '_') return C_MINUS;
            if (ch == ' ') return C_SPACE;
            return IGNORE;
        }
        putUTF8(ch, es->text);
        return TEXT;
    }
    else return nonText(es, shift, ctrl, code);
}

// Either mouse movement or scroll wheel.
static event mouseMove(events *es, ALLEGRO_EVENT *ae) {
    if (ae->mouse.dz != 0) {
        es->y = ae->mouse.z;
        return SCROLL;
    }
    es->x = ae->mouse.x;
    es->y = ae->mouse.y;
    if (es->mouseButtonDown) return DRAG;
    else return IGNORE;
}

// Ignore all buttons except first, for now.
static event mouseButton(events *es, ALLEGRO_EVENT *ae, bool down) {
    if (ae->mouse.button != 1) return IGNORE;
    es->mouseButtonDown = down;
    if (down) return CLICK;
    return DRAG;
}

event nextEvent(events *es) {
    ALLEGRO_EVENT ae;
    al_wait_for_event(es->queue, &ae);
    switch(ae.type) {
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return QUIT;
        case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(es->display);
            return FRAME;
            case ALLEGRO_EVENT_TIMER:
    //            printf("tick\n");
                return IGNORE;
        case ALLEGRO_EVENT_KEY_CHAR:
            return keyboard(es, &ae);
        case ALLEGRO_EVENT_MOUSE_AXES:
            return mouseMove(es, &ae);
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            return mouseButton(es, &ae, true);
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            return mouseButton(es, &ae, false);
        default:
//            printf("e %d\n", ae.type);
            return IGNORE;
    }
}

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

#ifdef eventsTest

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
