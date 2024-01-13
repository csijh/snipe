// The Snipe editor is free and open source. See licence.txt.
#include "handler.h"
#include "unicode.h"
#include "check.h"
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
    int touchPoint;
};

handler *newHandler(void *window) {
    handler *h = malloc(sizeof(handler));
    h->window = window;
    h->queue = al_create_event_queue();
    check(h->queue != 0, "Failed to create Allegro event queue.");
    check(al_install_keyboard(), "Failed to initialize keyboard.");
    check(al_install_mouse(), "failed to initialize mouse.");
    check(al_install_touch_input(), "failed to initialize touchpad");
    al_register_event_source(h->queue, al_get_display_event_source(window));
    al_register_event_source(h->queue, al_get_keyboard_event_source());
    al_register_event_source(h->queue, al_get_mouse_event_source());
    al_register_event_source(h->queue, al_get_touch_input_event_source());
    h->timer = al_create_timer(2.0);
    check(h->timer != NULL, "failed to create Allegro timer.");
    al_register_event_source(h->queue, al_get_timer_event_source(h->timer));
//    al_set_mouse_emulation_mode(ALLEGRO_MOUSE_EMULATION_TRANSPARENT);
    al_start_timer(h->timer);
    h->mouseButtonDown = h->shiftDown = h->ctrlDown = false;
    h->touchPoint = 0;
    return h;
}

void freeHandler(handler *h) {
    al_destroy_timer(h->timer);
    al_destroy_event_queue(h->queue);
    al_uninstall_touch_input();
    al_uninstall_mouse();
    al_uninstall_keyboard();
    free(h);
}

char *getEventText(handler *h) { return h->text; }
int getEventX(handler *h) { return h->x; }
int getEventY(handler *h) { return h->y; }

// Convert a non-text Allegro keycode into an event. Handle C_SPACE, C_0... and
// keypad keys.
static event nonText(handler *h, bool shift, bool ctrl, int code) {
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
            putUTF8('0' + (code - ALLEGRO_KEY_PAD_0), h->text);
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
    putUTF8(ch, h->text);
    return TEXT;
}

// Translate a keyboard event. Note Allegro (unusually) generates a KEY_CHAR
// event for every keypress, so KEY_DOWN is never needed, and KEY_UP is not
// needed unless actually tracking a long key press. (The 'feature' that
// C_A..C_Z produce unichar 1..26 isn't used, because it mucks up C_ENTER etc.
// and KEYPAD keys must explicitly be sent to nonText.)
static event keyboard(handler *h, ALLEGRO_EVENT *ae) {
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
        putUTF8(ch, h->text);
        return TEXT;
    }
    else return nonText(h, shift, ctrl, code);
}

// Either mouse movement or scroll wheel.
static event mouseMove(handler *h, ALLEGRO_EVENT *ae) {
    if (ae->mouse.dz != 0) {
        h->y = ae->mouse.dz;
        return SCROLL;
    }
    h->x = ae->mouse.x;
    h->y = ae->mouse.y;
    if (h->mouseButtonDown) return DRAG;
    else return IGNORE;
}

// Ignore all buttons except first, for now.
static event mouseButton(handler *h, ALLEGRO_EVENT *ae, bool down) {
    if (ae->mouse.button != 1) return IGNORE;
    h->mouseButtonDown = down;
    if (down && h->shiftDown && h->ctrlDown) return SC_CLICK;
    if (down && h->shiftDown) return S_CLICK;
    if (down && h->ctrlDown) return C_CLICK;
    if (down) return CLICK;
    if (h->shiftDown && h->ctrlDown) return SC_DRAG;
    if (h->shiftDown) return S_DRAG;
    if (h->ctrlDown) return C_DRAG;
    return DRAG;
}

// Track shift and ctrl keys being pressed.
static event trackModifiersDown(handler *h, ALLEGRO_EVENT *ae) {
    if (ae->keyboard.keycode == ALLEGRO_KEY_LSHIFT ||
        ae->keyboard.keycode == ALLEGRO_KEY_RSHIFT) {
        h->shiftDown = true;
    }
    if (ae->keyboard.keycode == ALLEGRO_KEY_LCTRL ||
        ae->keyboard.keycode == ALLEGRO_KEY_RCTRL) {
        h->ctrlDown = true;
    }
    return IGNORE;
}

// Track shift and ctrl keys being released.
static event trackModifiersUp(handler *h, ALLEGRO_EVENT *ae) {
    if (ae->keyboard.keycode == ALLEGRO_KEY_LSHIFT ||
        ae->keyboard.keycode == ALLEGRO_KEY_RSHIFT) {
        h->shiftDown = false;
    }
    if (ae->keyboard.keycode == ALLEGRO_KEY_LCTRL ||
        ae->keyboard.keycode == ALLEGRO_KEY_RCTRL) {
        h->ctrlDown = false;
    }
    return IGNORE;
}

event getNextEvent(handler *h) {
    ALLEGRO_EVENT ae;
    al_wait_for_event(h->queue, &ae);
    switch(ae.type) {
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return QUIT;
        case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(h->window);
            return RESIZE;
        case ALLEGRO_EVENT_DISPLAY_EXPOSE:
        case ALLEGRO_EVENT_DISPLAY_FOUND:
        case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
            return FRAME;
        case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
            h->shiftDown = h->ctrlDown = h->mouseButtonDown = false;
            return IGNORE;
        case ALLEGRO_EVENT_TIMER:
            printf("tick\n");
            return IGNORE;
        case ALLEGRO_EVENT_KEY_CHAR:
            return keyboard(h, &ae);
        case ALLEGRO_EVENT_KEY_DOWN:
            return trackModifiersDown(h, &ae);
        case ALLEGRO_EVENT_KEY_UP:
            return trackModifiersUp(h, &ae);
        case ALLEGRO_EVENT_MOUSE_AXES:
            return mouseMove(h, &ae);
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            return mouseButton(h, &ae, true);
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            return mouseButton(h, &ae, false);
        case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
            return FOCUS;
        case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
            return DEFOCUS;
        case ALLEGRO_EVENT_TOUCH_BEGIN:
            h->touchPoint = ae.touch.y;
            return IGNORE;
        case ALLEGRO_EVENT_TOUCH_MOVE:
            h->y = ae.touch.y - h->touchPoint;
            h->touchPoint = ae.touch.y;
            return SCROLL;
        default:
//            printf("e %d\n", ae.type);
            return IGNORE;
    }
}

#ifdef handlerTest

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    check(al_init(), "Failed to initialize Allegro.");
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
    ALLEGRO_DISPLAY *window = al_create_display(400, 300);
    check(window != NULL, "Failed to create window.");
    handler *h = newHandler(window);
    bool running = true;
    while (running) {
        event e = getNextEvent(h);
        if (e == QUIT) running = false;
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
