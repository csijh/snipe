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
    action controlText[128];
    action plain[COUNT_EVENTS];
    action shift[COUNT_EVENTS];
    action control[COUNT_EVENTS];
    action shiftControl[COUNT_EVENTS];
};

static void fixDefaults(map *m) {
    for (event e = 0; e < COUNT_EVENTS; e++) {
        if (m->shift[e] == Ignore) m->shift[e] = m->plain[e];
        if (m->control[e] == Ignore) m->control[e] = m->plain[e];
        if (m->shiftControl[e] == Ignore) m->shiftControl[e] = m->control[e];
    }
}

static void makeEntry(map *m, char *eventName, char *actionName) {
    action a = findAction(actionName);
    event flag = 0;
    if (strncmp(eventName, "S_", 2) == 0) { flag = S_; eventName += 2; }
    if (strncmp(eventName, "C_", 2) == 0) { flag = C_; eventName += 2; }
    if (strncmp(eventName, "SC_", 3) == 0) { flag = SC_; eventName += 3; }
    if (strlen(eventName) == 1) {
        assert(flag == C_);
        int ch = eventName[0];
        assert('!' <= ch && ch <= '~');
        m->controlText[ch] = a;
    }
    else {
        event e = findEvent(eventName);
        if (flag == SC_) m->shiftControl[e] = a;
        else if (flag == S_) m->shift[e] = a;
        else if (flag == C_) m->control[e] = a;
        else m->plain[e] = a;
    }
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
    for (int i = 0; i < 128; i++) m->controlText[i] = Ignore;
    for (int i = 0; i < COUNT_EVENTS; i++) m->plain[i] = Ignore;
    for (int i = 0; i < COUNT_EVENTS; i++) m->shift[i] = Ignore;
    for (int i = 0; i < COUNT_EVENTS; i++) m->control[i] = Ignore;
    for (int i = 0; i < COUNT_EVENTS; i++) m->shiftControl[i] = Ignore;
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
    action a;
    event base = clearEventFlags(e);
    if (e == addEventFlag(C_, TEXT)) a = m->controlText[(int)t[0]];
    else if (hasEventFlag(SC_, e)) a = m->shiftControl[base];
    else if (hasEventFlag(S_, e)) a = m->shift[base];
    else if (hasEventFlag(C_, e)) a = m->control[base];
    else a = m->plain[base];
    if (m->testing && e != BLINK && e != SAVE && e != REDRAW && e != TICK) {
        printEvent(e, r, c, t);
        printf("  ->  ");
        printAction(a);
    }
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
