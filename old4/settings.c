// Snipe settings. Free and open source. See licence.txt.
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct settings {
    char *text;
    char **lines;
    char ***variables;
};

static void splitLines(settings *ss) {
    char *text = ss->text;
    int n = 0;
    for (int i = 0; text[i] != '\0'; i++) if (text[i] == '\n') n++;
    ss->lines = malloc((n+1) * sizeof(char *));
    ss->lines[n] = NULL;
    int row = 0;
    char *line = text;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i] = '\0';
        if (i > 0 && text[i-1] == '\r') text[i-1] = '\0';
        ss->lines[row++] = line;
        line = &text[i+1];
    }
    ss->variables = malloc((n+1) * sizeof(char **));
    ss->variables[n] = NULL;
}

static void splitWords(settings *ss, int r) {
    char *line = ss->lines[r];
    if (line[0] == '#') { ss->variables[r] = NULL; return; }
    int n = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == ' ') continue;
        if (i == 0 || line[i-1] == ' ') n++;
    }
    if (n == 0) { ss->variables[r] = NULL; return; }
    ss->variables[r] = malloc((n+1) * sizeof(char *));
    ss->variables[r][n] = NULL;
    int w = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == ' ') { line[i] = '\0'; continue; }
        if (i == 0 || line[i-1] == '\0') ss->variables[r][w++] = &line[i];
    }
    if (n < 2 || strcmp(ss->variables[r][1], "=") != 0) {
        crash("bad setting in settings.txt line %d", r+1);
    }
}

settings *newSettings(files *fs) {
    settings *ss = malloc(sizeof(settings));
    char *filename = join(2, installDir(fs), "settings.txt");
    ss->text = readPath(filename);
    free(filename);
    splitLines(ss);
    for (int r = 0; ss->lines[r] != NULL; r++) {
        splitWords(ss, r);
    }
    return ss;
}

void freeSettings(settings *ss) {
    for (int r = 0; ss->lines[r] != NULL; r++) {
        if (ss->variables[r] != NULL) free(ss->variables[r]);
    }
    free(ss->variables);
    free(ss->lines);
    free(ss->text);
    free(ss);
}

// Get the i'th value of the given variable, or NULL.
char const *getSetting(settings *ss, char const *v, int i) {
    for (int r = 0; ss->lines[r] != NULL; r++) {
        if (ss->variables[r] == NULL) continue;
        if (strcmp(ss->variables[r][0], v) != 0) continue;
        return ss->variables[r][2+i];
    }
    crash("no setting found for %s", v);
    return NULL;
}

#ifdef TESTsettings

int main(int n, char const *args[]) {
    files *fs = newFiles(args[0]);
    settings *ss = newSettings(fs);
    assert(strcmp(getSetting(ss, "points", 0), "18") == 0);
    assert(strcmp(getSetting(ss, "rows", 0), "24") == 0);
    assert(strcmp(getSetting(ss, "columns", 0), "80") == 0);
    assert(strcmp(getSetting(ss, "help", 0), "open") == 0);
    assert(strcmp(getSetting(ss, "help", 1), "start") == 0);
    assert(strcmp(getSetting(ss, "help", 2), "chrome") == 0);
    freeSettings(ss);
    freeFiles(fs);
    printf("Settings module OK\n");
    return 0;
}

#endif
