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
};

static void fixDefaults(map *m) {
    for (event e = 0; e < COUNT_EVENTS / 4; e++) {
        if (m->array[S_ + e] == Ignore) m->array[S_ + e] = m->array[e];
        if (m->array[C_ + e] == Ignore) m->array[C_ + e] = m->array[e];
        if (m->array[SC_ + e] == Ignore) m->array[SC_ + e] = m->array[e];
    }
}

static void makeEntry(map *m, char *eventName, char *actionName) {
    event e = findEvent(eventName);
    action a = findAction(actionName);
    m->array[e] = a;
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
    for (int i = 0; i < length(lines); i++) {
        char *line = get(lines, i);
        if (! isalpha(line[0]) || line[0] == '\0') continue;
        strings *words = splitWords(line);
        char *eventName = get(words, 0);
        char *actionName = get(words, 1);
        makeEntry(m, eventName, actionName);
        freeStrings(words);
    }
    freeStrings(lines);
    free(content);
    fixDefaults(m);
    return m;
}

void freeMap(map *m) {
    free(m);
}

// Offer an action to the document, then the display, return whether quitting.
bool dispatch(map *m, event e, int r, int c, char *t) {
    action a = m->array[e];
    if (m->testing && e != BLINK && e != SAVE && e != REDRAW && e != TICK) {
        printEvent(e, r, c, t);
        printf("  ->  ");
        printAction(a);
    }
    event base = clearEventFlags(e);
    if (base == TEXT || base == CLICK || base == DRAG) {
        setData(m->doc, r, c, t);
    }
    actOnDocument(m->doc, a);
    actOnDisplay(m->dis, a);
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
