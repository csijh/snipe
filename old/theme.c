// The Snipe editor is free and open source, see licence.txt.
#include "theme.h"
#include "style.h"
#include "setting.h"
#include "file.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

// A colour holds RGBA data.
struct colour { unsigned char r, g, b, a; };

// A theme holds the list of names of theme files, and the current colour table.
struct theme {
    strings *names;
    int index;
    colour table[COUNT_STYLES];
};

// Add a string to a list.
static void add(strings *xs, char *s) {
    int n = length(xs);
    resize(xs, n + 1);
    S(xs)[n] = s;
}

theme *newTheme() {
    theme *t = malloc(sizeof(theme));
    t->names = newStrings();
    for (int i = 0; ; i++) {
        char *name = getThemeFile(i);
        if (name == NULL) break;
        add(t->names, name);
    }
    t->index = length(t->names) - 1;
    nextTheme(t);
    return t;
}

void freeTheme(theme *t) {
    freeList(t->names);
    free(t);
}

// Convert two hex characters into a byte.
static unsigned char hexByte(char *s) {
    char hex[] = "??";
    hex[0] = s[0];
    hex[1] = s[1];
    return (unsigned char)strtol(hex, NULL, 16);
}

void nextTheme(theme *t) {
    t->index = (t->index + 1) % length(t->names);
    char *filename = S(t->names)[t->index];
    char *path = resourcePath("", filename, "");
    char *content = readPath(path);
    free(path);
    normalize(content);
    strings *lines = splitLines(content);
    for (int s = 0; s < COUNT_STYLES; s++) t->table[s].a = 0;
    for (int i = 0; i < length(lines); i++) {
        char *line = S(lines)[i];
        if (! isalpha(line[0])) continue;
        strings *words = splitWords(line);
        char *styleName = S(words)[0];
        char *hexString = S(words)[1];
        int s = (int) findStyle(styleName);
        t->table[s].r = hexByte(&hexString[1]);
        t->table[s].g = hexByte(&hexString[3]);
        t->table[s].b = hexByte(&hexString[5]);
        t->table[s].a = 0xff;
        freeList(words);
    }
    freeList(lines);
    free(content);
    for (int s = 1; s < COUNT_STYLES; s++) {
        if (t->table[s].a != 0xff) t->table[s] = t->table[s - 1];
    }
}

extern inline colour *findColour(theme *t, char style) {
    colour *c = &t->table[(int)style];
    if (c->a == 0) c = &t->table[(int)styleDefault(style)];
    return c;
}

extern inline unsigned char red(colour *c) { return c->r; }
extern inline unsigned char green(colour *c) { return c->g; }
extern inline unsigned char blue(colour *c) { return c->b; }
extern inline unsigned char opacity(colour *c) { return c->a; }

#ifdef themeTest

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    theme *t = newTheme();
    colour *bg = &t->table[GAP];
    assert(bg->a == 0xff);
    int oldr = bg->r;
    nextTheme(t);
    assert(bg->r != oldr);
    freeTheme(t);
    printf("Theme module OK\n");
    return 0;
}

#endif
