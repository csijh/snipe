// Snipe event handling. Free and open source. See licence.txt.
#include "handler.h"
#include <allegro5/allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct handler {
    ALLEGRO_DISPLAY *window;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_TIMER *timer;
    char text[8]; int x, y;
    bool mouseButtonDown, shiftDown, ctrlDown;
};

handler *newHandler(void *window) {
    handler *es = malloc(sizeof(handler));
    es->window = window;
    es->queue = al_create_event_queue();
    try(es->queue != 0, "failed to create Allegro event queue.");
    try(al_install_keyboard(), "failed to initialize keyboard.");
    try(al_install_mouse(), "failed to initialize mouse.");
    try(al_install_touch_input(), "failed to initialize touchpad.");
    al_register_event_source(es->queue, al_get_display_event_source(window));
    al_register_event_source(es->queue, al_get_keyboard_event_source());
    al_register_event_source(es->queue, al_get_mouse_event_source());
    es->timer = al_create_timer(2.0);
    try(es->timer != NULL, "failed to create Allegro timer.");
    al_register_event_source(es->queue, al_get_timer_event_source(es->timer));
//    al_set_mouse_emulation_mode(ALLEGRO_MOUSE_EMULATION_TRANSPARENT);
    al_start_timer(es->timer);
    es->mouseButtonDown = es->shiftDown = es->ctrlDown = false;
    return es;
}

void freeHandler(handler *es) {
    al_destroy_timer(es->timer);
    al_destroy_event_queue(es->queue);
    al_uninstall_touch_input();
    al_uninstall_mouse();
    al_uninstall_keyboard();
    free(es);
}

char *getEventText(handler *es) { return es->text; }
int getEventX(handler *es) { return es->x; }
int getEventY(handler *es) { return es->y; }

// Convert a non-text Allegro keycode into an event. Handle C_SPACE, C_0... and
// keypad keys.
static event nonText(handler *es, bool shift, bool ctrl, int code) {
    int offset = shift ? (ctrl ? 3 : 1) : (ctrl ? 2 : 0) ;
    if (code >= ALLEGRO_KEY_A && code <= ALLEGRO_KEY_Z) {
        return C_A + (code - ALLEGRO_KEY_A) * 4 + offset;
    }
    if (code >= ALLEGRO_KEY_0 && code <= ALLEGRO_KEY_9) {
        return C_0 + (code - ALLEGRO_KEY_0);
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
static event keyboard(handler *es, ALLEGRO_EVENT *ae) {
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
            if (ch >= '0' && ch <= '9') return C_0 + ch - '0';
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
static event mouseMove(handler *es, ALLEGRO_EVENT *ae) {
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
static event mouseButton(handler *es, ALLEGRO_EVENT *ae, bool down) {
    if (ae->mouse.button != 1) return IGNORE;
    es->mouseButtonDown = down;
    printf("shift down %d\n", es->shiftDown);
    if (down && es->shiftDown && es->ctrlDown) return SC_CLICK;
    if (down && es->shiftDown) return S_CLICK;
    if (down && es->ctrlDown) return SC_CLICK;
    if (down) return CLICK;
    if (es->shiftDown && es->ctrlDown) return SC_DRAG;
    if (es->shiftDown) return S_DRAG;
    if (es->ctrlDown) return C_DRAG;
    return DRAG;
}

// Track shift and ctrl keys being pressed.
static event trackModifiersDown(handler *es, ALLEGRO_EVENT *ae) {
    if (ae->keyboard.keycode == ALLEGRO_KEY_LSHIFT ||
        ae->keyboard.keycode == ALLEGRO_KEY_RSHIFT) {
        es->shiftDown = true;
    }
    if (ae->keyboard.keycode == ALLEGRO_KEY_LCTRL ||
        ae->keyboard.keycode == ALLEGRO_KEY_RCTRL) {
        es->ctrlDown = true;
    }
    return IGNORE;
}

// Track shift and ctrl keys being released.
static event trackModifiersUp(handler *es, ALLEGRO_EVENT *ae) {
    if (ae->keyboard.keycode == ALLEGRO_KEY_LSHIFT ||
        ae->keyboard.keycode == ALLEGRO_KEY_RSHIFT) {
        es->shiftDown = false;
    }
    if (ae->keyboard.keycode == ALLEGRO_KEY_LCTRL ||
        ae->keyboard.keycode == ALLEGRO_KEY_RCTRL) {
        es->ctrlDown = false;
    }
    return IGNORE;
}

event getNextEvent(handler *es) {
    ALLEGRO_EVENT ae;
    al_wait_for_event(es->queue, &ae);
    switch(ae.type) {
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return QUIT;
        case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(es->window);
            return FRAME;
        case ALLEGRO_EVENT_TIMER:
            printf("tick\n");
            return IGNORE;
        case ALLEGRO_EVENT_KEY_CHAR:
            return keyboard(es, &ae);
        case ALLEGRO_EVENT_KEY_DOWN:
            return trackModifiersDown(es, &ae);
        case ALLEGRO_EVENT_KEY_UP:
            return trackModifiersUp(es, &ae);
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

#ifdef TESThandler

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    try(al_init(), "Failed to initialize Allegro.");
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
    ALLEGRO_DISPLAY *window = al_create_display(400, 300);
    try(window != NULL, "Failed to create window.");
    handler *h = newHandler(window);
    bool running = true;
    while (running) {
        event e = getNextEvent(h);
        if (e == QUIT) running = false;
        else if (e == FRAME) printf("FRAME\n");
        else if (e == TEXT) printf("TEXT %s\n", getEventText(h));
        else if (e == SCROLL) printf("SCROLL %d\n", getEventY(h));
        else if (e == CLICK) {
            printf("CLICK %d %d\n", getEventX(h), getEventY(h));
        }
        else if (e == DRAG) printf("DRAG %d %d\n", getEventX(h), getEventY(h));
        else if (e == IGNORE) continue;
        else printf("%s\n", findEventName(e));
    }
    freeHandler(h);
    al_destroy_display(window);
    printf("Handler module OK\n");
    return 0;
}

#endif
