// The Snipe editor is free and open source, see licence.txt.
#include "map.h"
#include "display.h"
#include "document.h"
#include "event.h"
#include "setting.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Get events from the display, translate them into actions using a map, execute
// the actions, then redraw the display.
struct snipe {
    display *dis;
    map *m;
    document *doc;
};
typedef struct snipe snipe;

static snipe *newSnipe(char const *path, bool testing) {
    snipe *s = malloc(sizeof(snipe));
    s->dis = newDisplay(path);
    s->doc = newDocument(path);
    s->m = newMap(s->doc, s->dis, testing);
    return s;
}

static void freeSnipe(snipe *s) {
    freeDisplay(s->dis);
    freeMap(s->m);
    freeDocument(s->doc);
    free(s);
}

// TODO: scrolling:
// - keep a scrolling target, top row to be displayed, in the document,
// - transfer the row height of the window to the document.
// - the scroll position takes account of cursors below the last line.
// - the scroll position tries to keep the main cursor in view.
// - PageUp/PageDown change the cursor position and scroll position
// - mouse or keyboard-scroll just change the scroll position.
// - the display scrolls smoothly towards the target position
// - the display scrolls faster if the target is further

// The runner thread executes this function. It takes and returns (void *) to
// match the pthreads convention. It is a pure event loop, because
// all timer ticks are included in the event stream. In particular, when
// scrolling, animation ticks make the loop act like an animation loop,
// otherwise it spends most of the time idle like a conventional event loop.
static void *run(void *vs) {
    snipe *s = (snipe *) vs;
    bool quitting = false;
    redraw(s->m);
    event e = FRAME;
    while (! quitting) {
        int r, c;
        char const *t;
//if (e != FRAME) printf("11\n");
        e = getEvent(s->dis, &r, &c, &t);
        if (e == PASTE) printf("PASTE %s\n", t);
//if (e != FRAME) { printf("9 "); printEvent(e, r, c, t); printf("\n"); }
        quitting = dispatch(s->m, e, r, c, t);
//if (e != FRAME) printf("10\n");
    }
    return NULL;
}

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    char *path;
    if (n == 1) path = fullPath(".");
    else if (n == 2) path = fullPath(args[1]);
    else {
        printf("Use: ./snipe filename\n");
        exit(1);
    }
    bool testing = strcmp(getSetting(Testing), "on") == 0;
    snipe *s = newSnipe(path, testing);
    startGraphics(s->dis, &run, s);
    free(path);
    freeSnipe(s);
    freeResources();
    return 0;
}
