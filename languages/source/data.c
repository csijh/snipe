// Snipe language compiler. Free and open source. See licence.txt.
#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char **splitLines(char *text) {
    int p = 0, row = 1;
    char **lines = newStrings();
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] != '\n') continue;
        text[i]= '\0';
        validateLine(row, &text[p]);
        addString(&lines, &text[p]);
        p = i + 1;
        row++;
    }
    return lines;
}

// Split a line into a list of tokens.
char **splitTokens(int row, char *line, char *tokens[]) {
    if (tokens == NULL) tokens = newStrings();
    int start = 0, end = 0, len = strlen(line);
    while (line[start] == ' ') start++;
    while (start < len) {
        end = start;
        while (line[end] != ' ' && line[end] != '\0') end++;
        line[end] = '\0';
        char *pattern = &line[start];
        addString(&tokens, pattern);
        start = end + 1;
        while (start < len && line[start] == ' ') start++;
    }
    return tokens;
}

#ifdef dataTest

int main(int argc, char const *argv[]) {
    char *text = readFile("data.c", false);
    printf("#chars in data.c = %d\n", (int) strlen(text));
    char **lines = splitLines(text);
    printf("#lines in data.c = %d\n", countStrings(lines));
    char **tokens = splitTokens(1, lines[0], NULL);
    printf("#tokens in data.c line 1 = %d\n", countStrings(tokens));
    free(tokens);
    free(lines);
    free(text);
    return 0;
}

#endif
