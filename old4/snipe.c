#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"

// Crash the program if there is any failure.
static void fail(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

int main(int n, char *args[]){
    display *d = newDisplay();
    files *fs;

    char *text;
    if (n < 2) fail("Use ./snipe file");
    fs = newFiles(args[0]);
    char *path = fullPath(fs, args[1]);
    text = readPath(path);
    unsigned char *tags = malloc(strlen(text) + 1);
    for (int i = 0; i < strlen(text); i++) {
        tags[i] = 4;
    }

    drawPage(d, text, tags);
    bool running = true;
    while (running) {
        event e = nextEvent(d);
        if (e == QUIT) running = false;
        else if (e == FRAME) drawPage(d, text, tags);
        else if (e == TEXT) printf("TEXT %s\n", eventText(d));
        else if (e == SCROLL) printf("SCROLL %d\n", eventY(d));
        else if (e == CLICK) {
            printf("CLICK %d %d\n", eventX(d), eventY(d));
            rowCol rc = findPosition(d, eventX(d), eventY(d));
            printf("R=%d, C=%d\n", rc.r, rc.c);
            drawCaret(d, rc.r, rc.c);
        }
        else if (e == DRAG) printf("DRAG %d %d\n", eventX(d), eventY(d));
        else if (e == IGNORE) continue;
        else printf("%s\n", findEventName(e));
    }

    freeDisplay(d);
    return 0;
}
