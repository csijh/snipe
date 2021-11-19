// Snipe language compiler. Free and open source. See licence.txt.
#include "strings.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

// If a string is a range, record the position of the dots, or -1, and the from
// and to values.
struct string { int length, dots, from, to; byte a[]; };

// Structure to return a UTF-8 character code and its number of bytes.
struct codelen { int code, length; };
typedef struct codelen codelen;

// Read a UTF-8 character code and length.
static codelen read(byte *in) {
    int ch = in[0], len = 1;
    if ((ch & 0x80) == 0) return (codelen) { ch, 1 };
    else if ((ch & 0xE0) == 0xC0) { len = 2; ch = ch & 0x3F; }
    else if ((ch & 0xF0) == 0xE0) { len = 3; ch = ch & 0x1F; }
    else if ((ch & 0xF8) == 0xF0) { len = 4; ch = ch & 0x0F; }
    for (int i = 1; i < len; i++) ch = (ch << 6) | (in[i] & 0x3F);
    return (codelen) { ch, len };
}

string *newString(int n, char *t) {
    string *s = malloc(sizeof(string) + n);
    s->length = n;
    s->dots = -1;
    strncpy((char *) s->a, t, n);
    return s;
}

void freeString(string *s) {
    free(s);
}

void checkPattern(string *s, int row) {
    int n = s->length;
    if (n > 127) crash("pattern too long on line %d", row);
    for (int i = 0; i < n - 1; i++) {
        if (s->a[i] == '.' && s->a[i+1] == '.') s->dots = i;
    }
    if (s->dots == 0) { s->from = 0; s->to = 1114111; }
    else if (s->dots > 0) {
        codelen cl = read(&s->a[0]);
        if (cl.length == s->dots) s->from = cl.code;
        else crash("bad range on line %d", row);
        cl = read(&s->a[s->dots+2]);
        if (s->dots + 2 + cl.length == s->length) s->to = cl.code;
        else crash("bad range on line %d", row);
    }
}

int length(string *s) {
    return s->length;
}

byte at(string *s, int i) {
    return s->a[i];
}

// Can't use strncmp because it stops at null.
int compare(string const *s1, string const *s2) {
    int n1 = s1->length, n2 = s2->length;
    for (int i = 0; i < n1 && i < n2; i++) {
        if (s1->a[i] < s2->a[i]) return -1;
        if (s1->a[i] > s2->a[i]) return 1;
    }
    if (n1 < n2) return -1;
    if (n1 > n2) return 1;
    return 0;
}

string *substring(string *s, int i, int j) {
    return newString(j - i, (char *) &s->a[i]);
}

// Write out a Unicode code point as UTF-8 bytes. Return length.
static int write(byte *out, int code, int row) {
    if (code <= 0x7F) {
        out[0] = code;
        return 1;
    }
    if (code <= 0x07FF) {
        out[0] = 0xC0 | ((code >> 6) & 0x1F);
        out[1] = 0x80 | (code & 0x3F);
        return 2;
    }
    if (code <= 0xFFFF) {
        out[0] = 0xE0 | ((code >> 12) & 0x0F);
        out[1] = 0x80 | ((code >> 6) & 0x3F);
        out[2] = 0x80 | (code & 0x3F);
        return 3;
    }
    if (code <= 0x10FFFF) {
        out[0] = 0xF0 | ((code >> 18) & 0x07);
        out[1] = 0x80 | ((code >> 12) & 0x3F);
        out[2] = 0x80 | ((code >> 6) & 0x3F);
        out[3] = 0x80 | (code & 0x3F);
        return 4;
    }
    crash("bad escape on line %d", row);
    return 0;
}

void unescape(string *s, int row) {
    int i = 0, j = 0, n = s->length;
    while (i < n) {
        if (s->a[i] != '\\') { s->a[j++] = s->a[i++]; continue; }
        i++;
        int ch = 0;
        while (i < n && isdigit(s->a[i])) ch = 10*ch + (s->a[i++] - '0');
        j = j + write(&s->a[j], ch, row);
    }
    s->length = j;
}

bool isRange(string *s, int row) {
    return s->dots >= 0;
}

int from(string *s) {
    return s->from;
}

int to(string *s) {
    return s->to;
}

string *newRange(int from, int to) {
    byte c[10];
    int n = write(c, from, 1);
    c[n++] = '.';
    c[n++] = '.';
    n = n + write(&c[n], to, 1);
    return newString(n, (char *)c);
}

string *readFile(char const *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) crash("can't read file %s", path);
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) crash("can't find size of file %s", path);
    int capacity = size + 2;
    string *s = malloc(sizeof(string) + capacity);
    int n = fread(s->a, 1, size, file);
    if (n != size) crash("can't read file %s", path);
    if (n > 0 && s->a[n - 1] != '\n') s->a[n++] = '\n';
    s->length = n;
    for (int i = 0; i < n; i++) {
        if (s->a[i] == '\n') continue;
        if (s->a[i] == '\t' || s->a[i] == '\r') s->a[i] = ' ';
        else if (s->a[i] < ' ') {
            crash("file %s contains control characters", path);
        }
    }
    fclose(file);
    return s;
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

#ifdef TESTstrings

int main() {
    string *s = newString(3, "cat");
    assert(length(s) == 3);
    assert(at(s,0) == 'c' && at(s,1) == 'a' && at(s,2) == 't');
    freeString(s);
    s = newString(4, "c\\0t");
    unescape(s,1);
    assert(length(s) == 3);
    assert(at(s,0) == 'c' && at(s,1) == '\0' && at(s,2) == 't');
    freeString(s);
    printf("Strings OK\n");
    return 0;
}

#endif
