#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "events.h"
#include "files.h"

// Crash the program if there is any failure.
static void fail(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

int main(int n, char *args[]){
    display *d = newDisplay();
    events *es = newEvents(getHandle(d));
    files *fs;

    char *text;
    if (n < 2) fail("Use ./snipe file");
    fs = newFiles(args[0]);
    char *path = fullPath(fs, args[1]);
    text = readPath(path);
    unsigned char *tags = malloc(strlen(text) + 1);
    for (int i = 0; i < strlen(text); i++) {
        if ((text[i] & 0xC0) == 0x80) tags[i] = 0;
        tags[i] = 1;
    }

    drawPage(d, text, tags);
    bool running = true;
    while (running) {
        event e = nextEvent(es);
        if (e == QUIT) running = false;
        else if (e == FRAME) drawPage(d, text, tags);
        else if (e == TEXT) printf("TEXT %s\n", eventText(es));
        else if (e == SCROLL) printf("SCROLL %d\n", eventY(es));
        else if (e == CLICK) printf("CLICK %d %d\n", eventX(es), eventY(es));
        else if (e == DRAG) printf("DRAG %d %d\n", eventX(es), eventY(es));
        else if (e == IGNORE) continue;
        else printf("%s\n", findEventName(e));
    }

    freeEvents(es);
    freeDisplay(d);
    return 0;
}
