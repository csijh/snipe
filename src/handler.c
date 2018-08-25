// The Snipe editor is free and open source, see licence.txt.
#include "handler.h"
#include "event.h"
#include "file.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <GLFW/glfw3.h>

// Event handling is implemented using GLFW. GLFW callbacks are converted into
// events using an explicit custom event queue (as in GLFW/GLEQ). Timer events,
// including animation frame events during scrolling, are included so that the
// main program loop becomes a pure event loop. During a long user gesture
// such as resizing the window or trackpad scrolling, the program freezes other
// than for the callbacks. So these callbacks are allowed to process the event
// inside the callback.

// The size of the event queue.
enum { SIZE = 1000 };

// Add a code for events which are to be discarded, and a code for a paste.
event IGNORE = COUNT_EVENTS;
event PASTE = COUNT_EVENTS + 1;

// An event structure holds a tag and two coordinates or a string or character.
struct position { int x, y; };
struct eventData {
    event tag;
    union { struct position p; char const *s; char c[5]; };
};
typedef struct eventData eventData;

struct handler {
    GLFWwindow *w;
    eventData queue[SIZE];
    int head, tail;
    double blinkRate, saveRate;
    double blinkTime, saveTime;
    bool frameTime, focused;
    dispatcher *dispatch;
    map *m;
};

// Prepare to add an event to the queue, returning a pointer to the next slot.
static eventData *push(handler *h) {
    eventData *e = &h->queue[h->head];
    h->head = (h->head + 1) % SIZE;
    assert(h->head != h->tail);
    return e;
}

bool focused(handler *h) {
    return h->focused;
}

// Generate a frame tick event, e.g. when smooth scrolling.
void frameTick(handler *h) {
    h->frameTime = true;
}

// Generate a paste event. Since events are handled synchronously, it should be
// OK to hang on to GLFW's pointer until the event is processed.
void pasteEvent(handler *h) {
    eventData *ed = push(h);
    ed->tag = PASTE;
    ed->s = glfwGetClipboardString(h->w);
    if (ed->s == NULL) ed->s = "";
}

// Copy text to the clipboard.
void clip(handler *h, char const *s) {
    glfwSetClipboardString(h->w, s);
}

// ----- Keyboard --------------------------------------------------------------
// Platforms provide character and key events. Character events are used for
// text input, because shift key information isn't provided in key events, and
// because it copes with Unicode character input. Key events are used for
// non-text input, including ctrl plus a text character, because some platforms
// don't produce corresponding character events. Key events for text, and
// character events for non-text, are ignored.

// A char event is the result of a user typing a text key. The platform handles
// the layout-dependent effect of the shift key. This could also be the
// completion of a platform input method for an ideographic language.
static void charCB(GLFWwindow *w, unsigned int unicode) {
    handler *h = glfwGetWindowUserPointer(w);
    eventData *ed = push(h);
    ed->tag = TEXT;
    putUTF8(unicode, ed->c);
}

// Convert a GLFW key to a character, or return -1, in a control key situation.
int getChar(int key) {
    if (GLFW_KEY_SPACE <= key && key <= GLFW_KEY_GRAVE_ACCENT) return key;
    switch(key) {
        case GLFW_KEY_KP_0: return '0';
        case GLFW_KEY_KP_1: return '1';
        case GLFW_KEY_KP_2: return '2';
        case GLFW_KEY_KP_3: return '3';
        case GLFW_KEY_KP_4: return '4';
        case GLFW_KEY_KP_5: return '5';
        case GLFW_KEY_KP_6: return '6';
        case GLFW_KEY_KP_7: return '7';
        case GLFW_KEY_KP_8: return '8';
        case GLFW_KEY_KP_9: return '9';
        case GLFW_KEY_KP_DECIMAL: return '.';
        case GLFW_KEY_KP_DIVIDE: return '/';
        case GLFW_KEY_KP_MULTIPLY: return '*';
        case GLFW_KEY_KP_SUBTRACT: return '-';
        case GLFW_KEY_KP_ADD: return '+';
        case GLFW_KEY_KP_EQUAL: return '=';
    }
    return -1;
}

// Create a control text event or return false. Since there is no keyboard
// layout information for these events, shift is ignored. For example control
// with 'a' or 'A' both produce C_A, and control with plus generates C_= on a
// keyboard where plus is a shifted equals.
static bool controlText(handler *h, int key) {
    char ch = getChar(key);
    if (ch < 0) return false;
    eventData *ed = push(h);
    ed->tag = addEventFlag(C_, ch);
    return true;
}

// Convert a key symbol for a non-text key into an event code or return -1.
static event nonText(int key) {
    switch (key) {
        case GLFW_KEY_WORLD_1: return WORLD_1;
        case GLFW_KEY_WORLD_2: return WORLD_1;
        case GLFW_KEY_ESCAPE: return ESCAPE;
        case GLFW_KEY_ENTER: return ENTER;
        case GLFW_KEY_KP_ENTER: return ENTER;
        case GLFW_KEY_TAB: return TAB;
        case GLFW_KEY_BACKSPACE: return BACKSPACE;
        case GLFW_KEY_INSERT: return INSERT;
        case GLFW_KEY_DELETE: return DELETE;
        case GLFW_KEY_RIGHT: return RIGHT;
        case GLFW_KEY_LEFT: return LEFT;
        case GLFW_KEY_DOWN: return DOWN;
        case GLFW_KEY_UP: return UP;
        case GLFW_KEY_PAGE_UP: return PAGE_UP;
        case GLFW_KEY_PAGE_DOWN: return PAGE_DOWN;
        case GLFW_KEY_HOME: return HOME;
        case GLFW_KEY_END: return END;
        case GLFW_KEY_F1: return F1;
        case GLFW_KEY_F2: return F2;
        case GLFW_KEY_F3: return F3;
        case GLFW_KEY_F4: return F4;
        case GLFW_KEY_F5: return F5;
        case GLFW_KEY_F6: return F6;
        case GLFW_KEY_F7: return F7;
        case GLFW_KEY_F8: return F8;
        case GLFW_KEY_F9: return F9;
        case GLFW_KEY_F10: return F10;
        case GLFW_KEY_F11: return F11;
        case GLFW_KEY_F12: return F12;
        case GLFW_KEY_F13: return F13;
        case GLFW_KEY_F14: return F14;
        case GLFW_KEY_F15: return F15;
        case GLFW_KEY_F16: return F16;
        case GLFW_KEY_F17: return F17;
        case GLFW_KEY_F18: return F18;
        case GLFW_KEY_F19: return F19;
        case GLFW_KEY_F20: return F20;
        case GLFW_KEY_F21: return F21;
        case GLFW_KEY_F22: return F22;
        case GLFW_KEY_F23: return F23;
        case GLFW_KEY_F24: return F24;
        case GLFW_KEY_F25: return F25;
        default: return -1;
    }
}

// Callback function for key events, to handle non-text keys and control
// combinations. Report only key-down or repeat events. GLFW_MOD_SUPER is the
// Cmd key on Macs.
static void keyCB(GLFWwindow *w, int key, int code, int action, int mods) {
    if (action == GLFW_RELEASE) return;
    handler *h = glfwGetWindowUserPointer(w);
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    bool ctrl = (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER)) != 0;
    if (ctrl && controlText(h, key)) return;
    event e = nonText(key);
    if (e < 0) return;
    if (shift) e = addEventFlag(S_, e);
    if (ctrl) e = addEventFlag(C_, e);
    eventData *ed = push(h);
    ed->tag = e;
}

// ---- Mouse ------------------------------------------------------------------

// Callback for mouse clicks. Ignore the right button for now. A mouse down
// event is reported as a CLICK, and a mouse up event as a DRAG. It is up to the
// caller to check whether the row/col position has changed in between.
static void mouseCB(GLFWwindow *w, int button, int action, int mods) {
    if (button != 0) return;
    handler *h = glfwGetWindowUserPointer(w);
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    bool ctrl = (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER)) != 0;
    event e = (action == GLFW_PRESS) ? CLICK : DRAG;
    if (shift) e = addEventFlag(S_, e);
    if (ctrl) e = addEventFlag(C_, e);
    eventData *ed = push(h);
    ed->tag = e;
    double dx, dy;
    glfwGetCursorPos(w, &dx, &dy);
    ed->p.x = (int) dx;
    ed->p.y = (int) dy;
}

// Callback for scroll wheel or touchpad scroll gesture events. Modifier info is
// not directly available, so ask for it.
static void scrollCB(GLFWwindow *w, double x, double y) {
    handler *h = glfwGetWindowUserPointer(w);
    bool shift =
        glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    bool ctrl =
        glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(w, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(w, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(w, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
    event e;
    if (y > 0) e = LINE_UP;
    else e = LINE_DOWN;
    if (shift) e = addEventFlag(S_, e);
    if (ctrl) e = addEventFlag(C_, e);
    h->dispatch(h->m, e, (int) x, (int) y, NULL);
//    eventData *ed = push(h);
//    ed->tag = e;
//    ed->p.x = (int) x;
//    ed->p.y = (int) y;
}

// ---- Window -----------------------------------------------------------------

// Callback for pressing the close-window button.
static void closeCB(GLFWwindow *w) {
    handler *h = glfwGetWindowUserPointer(w);
    eventData *ed = push(h);
    ed->tag = QUIT;
}

// Callback for resize. The main thread is often frozen bar callbacks during resize,
// so arrange for an immediate redraw within the callback.
static void resizeCB(GLFWwindow *w, int width, int height) {
    handler *h = glfwGetWindowUserPointer(w);
    h->dispatch(h->m, RESIZE, 0, 0, NULL);
//    eventData *ed = push(h);
//    ed->tag = RESIZE;
}

// Generate one last blink event before they are switched off, to make sure the
// display makes the cursor invisible.
static void focusCB(GLFWwindow *w, int focused) {
    handler *h = glfwGetWindowUserPointer(w);
    h->focused = focused;
    eventData *ed = push(h);
    ed->tag = BLINK;
}

// Create handler and set up all the desired callbacks.
handler *newHandler(void *w) {
    handler *h = malloc(sizeof(handler));
    h->w = w;
    h->head = h->tail = 0;
    h->blinkRate = 0.5;
    h->saveRate = 60.0;
    h->blinkTime = h->blinkRate;
    h->saveTime = h->saveRate;
    h->frameTime = false;
    h->focused = true;
    glfwSetWindowUserPointer(h->w, h);
    glfwSetKeyCallback(h->w, keyCB);
    glfwSetCharCallback(h->w, charCB);
    glfwSetMouseButtonCallback(h->w, mouseCB);
    glfwSetScrollCallback(h->w, scrollCB);
    glfwSetWindowCloseCallback(h->w, closeCB);
//    glfwSetWindowSizeCallback(h->w, resizeCB);
    glfwSetWindowFocusCallback(h->w, focusCB);
    return h;
}

void freeHandler(handler *h) {
    free(h);
}

void setCallback(handler *h, dispatcher *f, map *m) {
    h->dispatch = f;
    h->m = m;
    glfwSetWindowSizeCallback(h->w, resizeCB);

}

void setBlinkRate(handler *h, double br) {
    h->blinkRate = br;
}

// Get the next event, possibly with a pause. Note that glfwWaitEventsTimeout
// returns early, even e.g. for mouse movements with no callback.
event getRawEvent(handler *h, int *x, int *y, char const **t) {
    while (true) {
        glfwPollEvents();
        if (h->head != h->tail) {
            eventData *e = &h->queue[h->tail];
            h->tail = (h->tail + 1) % SIZE;
            event base = clearEventFlags(e->tag);
            if (base == TEXT) {
                *x = *y = 0;
                *t = e->c;
            }
            else if (base == PASTE) {
                e->tag = TEXT;
                *x = *y = 0;
                *t = e->s;
            }
            else if (base == CLICK || base == DRAG) {
                *x = e->p.x;
                *y = e->p.y;
                *t = "";
                e->c[0] = '\0';
            }
            else {
                *x = *y = 0;
                *t = "";
            }
            return e->tag;
        }
        double time = glfwGetTime();
        if (h->frameTime) { h->frameTime = false; return TICK; }
        if (h->focused && h->blinkRate > 0 && time > h->blinkTime) {
            if (h->blinkTime < time - h->blinkRate) h->blinkTime = time;
            h->blinkTime += h->blinkRate; return BLINK;
        }
        if (time > h->saveTime) {
            h->saveTime += h->saveRate;
            return SAVE;
        }
        double min = h->saveTime;
        if (h->focused && h->blinkRate > 0 && h->blinkTime < h->saveTime) {
            min = h->blinkTime;
        }
        glfwWaitEventsTimeout(min - time);
    }
}

#ifdef test_handler

// Testing and error handling are in the display module.
int main() {
    printf("Test interactively via the display module.\n");
    printf("Handler module OK\n");
    return 0;
}

#endif
