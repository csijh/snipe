#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>

static void err(char *e, char const *p) { printf("Error, %s: %s\n", e, p); }

// Find the size of a text file, or -1.
int sizeFile(char const *path) {
    struct stat info;
    int result = stat(path, &info);
    if (result < 0) return -1;
    if (! S_ISREG(info.st_mode)) return -1;
    if (info.st_size >= INT_MAX) return -1;
    int size = (int) info.st_size;
    return size;
}

// Use binary mode, so that the number of bytes read equals the file size.
static char *readFile(char const *path) {
    assert(path[strlen(path) - 1] != '/');
    int size = sizeFile(path);
    if (size < 0) { err("can't read", path); return NULL; }
    FILE *file = fopen(path, "rb");
    if (file == NULL) { err("can't read", path); return NULL; }
    char *data = malloc(size + 2);
    int n = fread(data, 1, size, file);
    if (n != size) { free(data); err("read failed", path); return NULL; }
    if (n > 0 && data[n - 1] != '\n') data[n++] = '\n';
    data[n] = '\0';
    fclose(file);
    return data;
}

int main(int argc, char const *argv[]) {
    clock_t t0 = clock();
    char *buffer = readFile("unicode/UnicodeData.txt");
    clock_t t1 = clock();
    int n = strlen(buffer);
    clock_t t2 = clock();
    bool ok = uvalid(n, buffer, true);
    clock_t t3 = clock();
    printf("CPS = %ld\n", CLOCKS_PER_SEC);
    printf("t0 = %ld\n", t0);
    printf("t1 = %ld\n", t1);
    printf("t2 = %ld\n", t2);
    printf("t3 = %ld\n", t3);
    printf("ok = %d\n", ok);
    /* code */
    return 0;
}
