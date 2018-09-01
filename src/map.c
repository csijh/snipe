// The Snipe editor is free and open source, see licence.txt.
#include "map.h"
#include "setting.h"
#include "event.h"
#include "action.h"
#include "file.h"
#include "string.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

// A map has arrays of actions, indexed by character or event.
struct map {
    document *doc;
    display *dis;
    bool testing;
    action array[COUNT_EVENTS];
    action listArray[COUNT_EVENTS];
};

static void fixDefaults(map *m) {
    for (event e = 0; e < COUNT_EVENTS / 4; e++) {
        if (m->array[S_ + e] == Ignore) m->array[S_ + e] = m->array[e];
        if (m->array[C_ + e] == Ignore) m->array[C_ + e] = m->array[e];
        if (m->array[SC_ + e] == Ignore) m->array[SC_ + e] = m->array[e];
    }
}

static void makeEntry(map *m, bool list, char *eventName, char *actionName) {
    event e = findEvent(eventName);
    action a = findAction(actionName);
    if (list) m->listArray[e] = a;
    else m->array[e] = a;
}

map *newMap(document *doc, display *dis, bool testing) {
    char *file = getSetting(Map);
    char *path = resourcePath("", file, "");
    char *content = readPath(path);
    free(path);
    normalize(content);
    strings *lines = splitLines(content);
    map *m = malloc(sizeof(map));
    m->doc = doc;
    m->dis = dis;
    m->testing = testing;
    for (event e = 0; e < COUNT_EVENTS; e++) m->array[e] = Ignore;
    for (event e = 0; e < COUNT_EVENTS; e++) m->listArray[e] = Ignore;
    for (int i = 0; i < length(lines); i++) {
        char *line = S(lines)[i];
        if (! isalpha(line[0]) || line[0] == '\0') continue;
        strings *words = splitWords(line);
        bool list = false;
        char *eventName = S(words)[0];
        char *actionName = S(words)[1];
        if (strcmp(eventName, "List") == 0) {
            list = true;
            eventName = actionName;
            actionName = S(words)[2];
        }
        makeEntry(m, list, eventName, actionName);
        freeList(words);
    }
    freeList(lines);
    free(content);
    fixDefaults(m);
    return m;
}

void freeMap(map *m) {
    free(m);
}

// Redraw the window after a change of any kind. Transfer the scroll target,
// text, and style to the display.
void redraw(map *m) {
    int height = getHeight(m->doc);
    for (int r = firstRow(m->dis); r <= lastRow(m->dis); r++) {
        if (r > height) break;
        int n = getWidth(m->doc, r);
        chars *line = getLine(m->doc, r);
        chars *st = getStyle(m->doc, r);
        addCursorFlags(m->doc, r, n, st);
        drawLine(m->dis, r, length(line), C(line), C(st));
    }
    showFrame(m->dis);
}

// Offer an action to the document, then the display, return whether quitting.
bool dispatch(map *m, event e, int x, int y, char const *t) {
    action a;
    if (isDirectory(m->doc)) a = m->listArray[e];
    else a = m->array[e];
    if (m->testing && e != BLINK && e != SAVE && e != RESIZE && e != FRAME) {
        printEvent(e, x, y, t, "");
        printf("  ->  ");
        printAction(a);
    }
    event base = clearEventFlags(e);
    if (base == CLICK || base == DRAG) {
        int r, c;
        charPosition(m->dis, x, y, &r, &c);
        setRowColData(m->doc, r, c);
    }
    else if (base == TEXT || base == PASTE) {
        setTextData(m->doc, t);
    }
    char const *copy = actOnDocument(m->doc, a);
    setDocRows(m->dis, getHeight(m->doc));
    actOnDisplay(m->dis, a, x, y, copy);
    if (a == Open || a == Load) setTitle(m->dis, getPath(m->doc));
    if (a == Frame) redraw(m);
    return a == Quit;
}

#ifdef test_map

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    map *m = newMap(NULL, NULL, true);
    freeMap(m);
    printf("Map module OK\n");
    return 0;
}

#endif
