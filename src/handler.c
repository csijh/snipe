// The Snipe editor is free and open source, see licence.txt.
#define _POSIX_C_SOURCE 200809L
#include "handler.h"
#include "queue.h"
#include "event.h"
#include "file.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <GLFW/glfw3.h>

// Event handling is implemented using GLFW. GLFW callbacks put events on the
// queue (as in GLFW/GLEQ).

// Add a code for events which are to be discarded.
event IGNORE = COUNT_EVENTS;

struct handler {
    GLFWwindow *w;
    double blinkRate, saveRate;
    double blinkTime, saveTime;
    bool focused, alive;
    queue *q;
};

handler *newHandler(void *w) {
    handler *h = malloc(sizeof(handler));
    h->w = w;
    h->blinkRate = 0.5;
    h->saveRate = 60.0;
    h->blinkTime = h->blinkRate;
    h->saveTime = h->saveRate;
    h->focused = true;
    h->alive = true;
    h->q = newQueue();
    return h;
}

void freeHandler(handler *h) {
    freeQueue(h->q);
    free(h);
}

bool focused(handler *h) {
    return h->focused;
}

// Generate a frame event from the runner thread, e.g. when smooth scrolling.
void frameEvent(handler *h) {
    enqueue(h->q, TICK, 0, 0, NULL);
}

// Generate a paste event from the runner thread.
void pasteEvent(handler *h) {
    char const *s = glfwGetClipboardString(h->w);
    if (s == NULL) s = "";
    enqueue(h->q, PASTE, 0, 0, s);
}

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
    char text[5];
    putUTF8(unicode, text);
    handler *h = glfwGetWindowUserPointer(w);
    enqueue(h->q, TEXT, 0, 0, text);
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
    enqueue(h->q, addEventFlag(C_, ch), 0, 0, NULL);
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
    enqueue(h->q, e, 0, 0, NULL);
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
    double dx, dy;
    glfwGetCursorPos(w, &dx, &dy);
    enqueue(h->q, e, (int) dx, (int) dy, NULL);
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
    enqueue(h->q, e, (int) x, (int) y, NULL);
}

// ---- Window -----------------------------------------------------------------

// Callback for pressing the close-window button.
static void closeCB(GLFWwindow *w) {
    handler *h = glfwGetWindowUserPointer(w);
    enqueue(h->q, QUIT, 0, 0, NULL);
    h->alive = false;
}

// Callback for resize.
static void resizeCB(GLFWwindow *w, int width, int height) {
    handler *h = glfwGetWindowUserPointer(w);
    enqueue(h->q, RESIZE, 0, 0, NULL);
}

static void focusCB(GLFWwindow *w, int focused) {
    handler *h = glfwGetWindowUserPointer(w);
    h->focused = focused;
}

// Set up all the desired callbacks, then process events in a loop.
void handle(handler *h) {
    glfwSetWindowUserPointer(h->w, h);
    glfwSetKeyCallback(h->w, keyCB);
    glfwSetCharCallback(h->w, charCB);
    glfwSetMouseButtonCallback(h->w, mouseCB);
    glfwSetScrollCallback(h->w, scrollCB);
    glfwSetWindowCloseCallback(h->w, closeCB);
    glfwSetWindowFocusCallback(h->w, focusCB);
    glfwSetWindowSizeCallback(h->w, resizeCB);
    while (h->alive) glfwWaitEventsTimeout(1.0);
}

// Delay for a given number of seconds.
static void delay(double s) {
    struct timespec t;
    t.tv_sec = (int) s;
    s = s - (int) s;
    t.tv_nsec = (int) (s * 1000000000.0);
    nanosleep(&t, &t);
}

// Generate tick events in a loop.
void *tick(void *vh) {
    handler *h = (handler *) vh;
    while (h->alive) {
        double time = glfwGetTime();
        if (h->blinkRate > 0 && time > h->blinkTime) {
            enqueue(h->q, BLINK, 0, 0, NULL);
            h->blinkTime += h->blinkRate;
        }
        if (time > h->saveTime) {
            enqueue(h->q, SAVE, 0, 0, NULL);
            h->saveTime += h->saveRate;
        }
        double min = h->saveTime;
        if (h->blinkRate > 0 && h->blinkTime < h->saveTime) min = h->blinkTime;
        delay(min - time);
    }
    return NULL;
}

void setBlinkRate(handler *h, double br) {
    h->blinkRate = br;
}

event getRawEvent(handler *h, int *x, int *y, char const **t) {
    return dequeue(h->q, x, y, t);
}

#ifdef test_handler

#include <pthread.h>

// Represents the runner thread.
static void *run(void *vh) {
    handler *h = (handler *) vh;
    event e = BLINK;
    int x, y;
    char const *t;
    while (e != QUIT) {
        e = getRawEvent(h, &x, &y, &t);
        printEvent(e, x, y, t); printf("\n");
    }
    return NULL;
}

// Test
int main() {
    setbuf(stdout, NULL);
    printf("Check key, mouse, window events.\n");
    printf("Check blinks continue while moving/resizing/scrolling window.\n");
    glfwInit();
    GLFWwindow *gw = glfwCreateWindow(640, 480, "Test", NULL, NULL);
    handler *h = newHandler(gw);
    pthread_t runner, ticker;
    pthread_create(&runner, NULL, &run, h);
    pthread_create(&ticker, NULL, &tick, h);
    handle(h);
    pthread_join(runner, NULL);
    pthread_join(ticker, NULL);
    freeHandler(h);
    glfwDestroyWindow(gw);
    glfwTerminate();
    printf("Handler module OK\n");
    return 0;
}

#endif
