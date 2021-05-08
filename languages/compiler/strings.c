// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Copy and adapt this list implementation for any desired list type.
struct strings { int c, n; char **a; };
extern inline char *getString(strings *l, int i) { return l->a[i]; }
extern inline void setString(strings *l, int i, char *s) { l->a[i] = s; }
int countStrings(strings *l) { return l->n; }
void clearStrings(strings *l) { l->n = 0; }
void freeStrings(strings *l) { free(l->a); free(l); }

strings *newStrings() {
    strings *l = malloc(sizeof(strings));
    *l = (strings) { .n = 0, .c = 4, .a = malloc(4 * sizeof(char *)) };
    return l;
}

int addString(strings *l, char *s) {
    if (l->n >= l->c) {
        l->c = l->c * 2;
        l->a = realloc(l->a, l->c * sizeof(char *));
    }
    l->a[l->n] = s;
    return l->n++;
}

char *readFile(char const *path, bool binary) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("can't read file %s", path);
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("can't find file size of %s", path);
    int capacity = size + 2;
    if (binary) capacity = size;
    char *text = malloc(capacity);
    int n = fread(text, 1, size, file);
    if (n != size) crash("can't read file %s", path);
    if (! binary && n > 0 && text[n - 1] != '\n') text[n++] = '\n';
    if (! binary) text[n] = '\0';
    fclose(file);
    return text;
}

// Validate a line. Check it is ASCII only. Convert '\t' or '\r' to a space. Ban
// other control characters.
static void validateLine(int row, char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        unsigned char ch = line[i];
        if (ch == '\t' || ch == '\r') line[i] = ' ';
        else if (ch >= 128) crash("non-ASCII character on line %d", row);
        else if (ch < ' ' || ch > '~') crash("control char on line %d", row);
    }
}

void splitLines(char *text, strings *lines) {
    int p = 0, row = 1;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i]= '\0';
        validateLine(row, &text[p]);
        addString(lines, &text[p]);
        p = i + 1;
        row++;
    }
}

void splitTokens(int row, char *line, strings *tokens) {
    int start = 0, end = 0, len = strlen(line);
    while (line[start] == ' ') start++;
    while (start < len) {
        end = start;
        while (line[end] != ' ' && line[end] != '\0') end++;
        line[end] = '\0';
        char *token = &line[start];
        addString(tokens, token);
        start = end + 1;
        while (start < len && line[start] == ' ') start++;
    }
}

void crash(char const *fmt, ...) {
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

#ifdef stringsTest

int main() {
    char *text = readFile("strings.c", false);
    printf("#chars in data.c = %d\n", (int) strlen(text));
    strings *lines = newStrings();
    splitLines(text, lines);
    printf("#lines in data.c = %d\n", countStrings(lines));
    strings *tokens = newStrings();
    splitTokens(1, getString(lines,0), tokens);
    printf("#tokens in data.c line 1 = %d\n", countStrings(tokens));
    freeStrings(tokens);
    freeStrings(lines);
    free(text);
    return 0;
}

#endif
