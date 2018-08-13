// The Snipe editor is free and open source, see licence.txt.
#include "setting.h"
#include "string.h"
#include "list.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

// Declare fixed space for the setting strings.
enum { MAX_SETTINGS = 30, MAX_STRING = 50 };
static char values[MAX_SETTINGS][MAX_STRING];

static char *settingNames[] = {
    [Font]="Font", [FontSize]="FontSize", [Rows]="Rows", [Columns]="Columns",
    [BlinkRate]="BlinkRate", [MinScroll]="MinScroll", [MaxScroll]="MaxScroll",
    [HelpCommand]="Help", [HelpIndex]="HelpIndex", [Testing]="Testing",
    [Map]="Map", [Theme]="Theme"
};

// Convert the name of a setting to a setting constant, or -1 to ignore.
// Use platform macro variables defined by the Makefile.
static setting convert(char *name) {
    for (setting s = 0; s <= Theme; s++) {
        if (strcmp(name, settingNames[s]) == 0) return s;
    }
    if (strncmp(name, "Help", 4) != 0) {
        printf("Unknown setting name %s\n", name);
        exit(1);
    }
#ifdef __linux__
    if (strcmp(name, "HelpLinux") == 0) return HelpCommand;
#elif __APPLE__
    if (strcmp(name, "HelpMacOS") == 0) return HelpCommand;
#elif _WIN32
    if (strcmp(name, "HelpWindows") == 0) return HelpCommand;
#endif
    return -1;
}

static void error(char *s, char *w) {
    printf("%s %s\n", s, w);
    exit(1);
}

void readSettings() {
    char *path = resourcePath("", "settings", ".txt");
    char *content = readPath(path);
    normalize(content);
    free(path);
    strings *lines = splitLines(content);
    for (int i = 0; i < length(lines); i++) {
        char *line = S(lines)[i];
        if (! isalpha(line[0])) continue;
        int sp = strchr(line, ' ') - line;
        line[sp] = '\0';
        setting s = convert(line);
        if (s < 0) continue;
        if (s == Theme) while (values[s][0] != 0) s++;
        if (s >= MAX_SETTINGS - 1) error("Too many settings", "");
        char *value = &line[sp+1];
        while (value[0] == ' ') value++;
        if (strlen(value) >= MAX_STRING) error("String too long", value);
        strcpy(values[s], value);
    }
    freeList(lines);
    free(content);
}

char *getSetting(setting s) {
    if (values[0][0] == 0) readSettings();
    return values[s];
}

char *getThemeFile(int i) {
    if (values[0][0] == 0) readSettings();
    int t = Theme + i;
    if (values[t][0] == 0) return NULL;
    return values[t];
}

#ifdef test_setting

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    readSettings();
    assert(strcmp(getSetting(Font), "files/DejaVuSansMono.ttf") == 0);
    assert(strcmp(getSetting(Map), "files/map.txt") == 0);
    freeResources();
    printf("Setting module OK\n");
    return 0;
}

#endif
