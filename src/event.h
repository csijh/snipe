// The Snipe editor is free and open source. See licence.txt.
// TODO: touch scroll
//#include "settings.h"

// Event code constants represent keyboard, mouse, window and timer events.
// These constants are intended to be independent of the graphics library, and
// suitable for user-defined key maps.

// The prefixes S_ C_ and SC_ represent Shift, Control or both. On macOS, Cmd is
// treated the same as Control. Combinations using Alt are not included, because
// they are likely to be bound to system-wide operations.

// CLICK and DRAG are mouse button down and up events. They are accompanied by
// pixel coordinates, later converted into document row/column coordinates by
// the display module. Every CLICK is followed by a DRAG, whether the mouse has
// been moved or not - a DRAG event can be discarded later if the row/column
// position is the same as the CLICK.

// SCROLL events are generated by a mouse scroll wheel, or equivalent touchpad
// gesture, accompanied by a scroll amount. A TEXT event is accompanied by a
// character, possibly generated by a platform's Unicode input method. The
// codes after TEXT are window or timer events. FOCUS/DEFOCUS are when the
// mouse enters or leaves the display. FRAME is a general request for redraw,
// e.g. if the display is exposed.

// In combinations with CTRL plus a text character, SHIFT is problematic because
// of the lack of keyboard layout information, so it is ignored. For convenience
// on the majority of keyboards, the combination CTRL and = is interpreted as
// C_PLUS, and so is CTRL and keypad plus. CTRL and underscore, and CTRL and
// keypad minus, are interpreted as C_MINUS.

enum event {
    CLICK, S_CLICK, C_CLICK, SC_CLICK,
    DRAG, S_DRAG, C_DRAG, SC_DRAG,
    SCROLL, S_SCROLL, C_SCROLL, SC_SCROLL,
    ESCAPE, S_ESCAPE, C_ESCAPE, SC_ESCAPE,
    ENTER, S_ENTER, C_ENTER, SC_ENTER,
    TAB, S_TAB, C_TAB, SC_TAB,
    BACKSPACE, S_BACKSPACE, C_BACKSPACE, SC_BACKSPACE,
    INSERT, S_INSERT, C_INSERT, SC_INSERT,
    DELETE, S_DELETE, C_DELETE, SC_DELETE,
    RIGHT, S_RIGHT, C_RIGHT, SC_RIGHT,
    LEFT, S_LEFT, C_LEFT, SC_LEFT,
    DOWN, S_DOWN, C_DOWN, SC_DOWN,
    UP, S_UP, C_UP, SC_UP,
    PAGE_UP, S_PAGE_UP, C_PAGE_UP, SC_PAGE_UP,
    PAGE_DOWN, S_PAGE_DOWN, C_PAGE_DOWN, SC_PAGE_DOWN,
    HOME, S_HOME, C_HOME, SC_HOME,
    END, S_END, C_END, SC_END,
    F1, S_F1, C_F1, SC_F1, F2, S_F2, C_F2, SC_F2, F3, S_F3, C_F3, SC_F3,
    F4, S_F4, C_F4, SC_F4, F5, S_F5, C_F5, SC_F5, F6, S_F6, C_F6, SC_F6,
    F7, S_F7, C_F7, SC_F7, F8, S_F8, C_F8, SC_F8, F9, S_F9, C_F9, SC_F9,
    F10, S_F10, C_F10, SC_F10, F11, S_F11, C_F11, SC_F11,
    F12, S_F12, C_F12, SC_F12,
    MENU, S_MENU, C_MENU, SC_MENU,
    C_A, C_B, C_C, C_D, C_E, C_F, C_G, C_H, C_I, C_J, C_K, C_L, C_M, C_N, C_O,
    C_P, C_Q, C_R, C_S, C_T, C_U, C_V, C_W, C_X, C_Y, C_Z, C_0, C_1, C_2, C_3,
    C_4, C_5, C_6, C_7, C_8, C_9, C_SPACE, C_PLUS, C_MINUS,
    TEXT, PASTE, RESIZE, FOCUS, DEFOCUS,
    FRAME, LOAD, BLINK, SAVE, QUIT,
    IGNORE
};

typedef enum event event;

// Get the name of an event constant as a string (including combinations).
const char *findEventName(event e);

// Find an event from its name (including S_ or C_ or SC_ prefix).
event findEvent(char *name);

// Print out an event with a given terminating string, e.g. for testing.
void printEvent(event e, int r, int c, char const *t, char *end);
