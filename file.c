// The Snipe editor is free and open source, see licence.txt.

// Find the path to the installation directory from args[0]. This appears to be
// the only simple cross-platform technique which doesn't involve making an
// installer. Also find the current working directory on startup,

// Directory handling requires some Posix functions from unistd.h, dirent.h and
// sys/stat.h. See http://pubs.opengroup.org/onlinepubs/9699919799/. Forward
// slashes are used exclusively (which Windows libraries accept). File names
// must not contain / or \.
#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include "file.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

// The current working directory on startup, and the installation directory.
static char *current = NULL;
static char *install = NULL;

// Give an error message and stop.
static void crash(char const *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

// Get the current working directory, with trailing /.
static void findCurrent() {
    if (current != NULL) free(current);
    long size = 100;
    current = malloc(size);
    while (getcwd(current, size) == NULL) {
        size += 100;
        current = realloc(current, size);
    }
    int n = strlen(current);
    for (int i = 0; i < n; i++) if (current[i] == '\\') current[i] = '/';
    if (size < n + 2) current = realloc(current, n + 2);
    if (current[n - 1] != '/') strcat(current, "/");
}

// Check whether a path is absolute. Allow for a Windows drive letter prefix.
static bool absolute(char const *path) {
    int n = strlen(path);
    if (n >= 1 && path[0] == '/') return true;
    if (n >= 2 && path[1] == ':') return true;
    return false;
}

// Find the installation directory from args[0].
static void findInstall(char const *program) {
    if (install != NULL) free(install);
    int n = strlen(program) + 1;
    install = malloc(n);
    strcpy(install, program);
    for (int i = 0; i < n; i++) if (install[i] == '\\') install[i] = '/';
    if (! absolute(install)) {
        if (n >= 2 && install[0]=='.' && install[1]=='/') {
            memmove(install, install + 2, n - 2);
            n = n - 2;
        }
        char *s = malloc(strlen(current) + n);
        strcpy(s, current);
        strcat(s, install);
        free(install);
        install = s;
    }
    char *suffix = strrchr(install, '/');
    assert(suffix != NULL);
    suffix[1] = '\0';
}

void findResources(char const *program) {
    findCurrent();
    findInstall(program);
}

void freeResources() {
    free(current);
    free(install);
}

// Allocate a new string even if the path is absolute.
static char *addPath(char const *path, char const *file) {
    if (absolute(file)) path = "";
    int len = strlen(path) + strlen(file) + 1;
    char *s = malloc(len);
    strcpy(s, path);
    strcat(s, file);
    return s;
}

char *resourcePath(char *d, char *f, char *e) {
    if (install == NULL) crash("Must call findResources first");
    char *p = malloc(strlen(install) + strlen(d) + strlen(f) + strlen(e) + 1);
    strcpy(p, install);
    strcat(p, d);
    strcat(p, f);
    strcat(p, e);
    return p;
}

// Allocate a new string even if the path is absolute.
char *fullPath(char const *file) {
    if (current == NULL) crash("Must call findResources first");
    char *path = current;
    if (absolute(file)) path = "";
    return addPath(path, file);
}

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
    if (data[n - 1] != '\n') data[n++] = '\n';
    data[n] = '\0';
    fclose(file);
    return data;
}

// Compare two strings in natural order.
static int compare(char *s1, char *s2) {
    while (*s1 != '\0' || *s2 != '\0') {
        char c1 = *s1, c2 = *s2;
        if (! isdigit(c1) || ! isdigit(c2)) {
            if (c1 < c2) return -1;
            else if (c1 > c2) return 1;
            else { s1++; s2++; continue; }
        }
        int n1 = atoi(s1), n2 = atoi(s2);
        if (n1 < n2) return -1;
        else if (n1 > n2) return 1;
        while (isdigit(*s1)) s1++;
        while (isdigit(*s2)) s2++;
    }
    return 0;
}

// Sort strings into natural order.
static void sort(int n, char *ss[n]) {
    for (int i = 1; i < n; i++) {
        char *s = ss[i];
        int j = i - 1;
        while (j >= 0 && compare(ss[j], s) > 0) {
            ss[j+1] = ss[j];
            j--;
        }
        ss[j+1] = s;
    }
}

// Check if a directory entry is valid, rejecting "." and names with slashes.
static bool valid(char *name) {
    if (strcmp(name, ".") == 0) return false;
    if (strchr(name, '/') != NULL) return false;
    if (strchr(name, '\\') != NULL) return false;
    return true;
}

// Measure the number of entries and text length of a directory (including room
// for adding final slashes).
static void measureDirectory(DIR *dir, int *n, int *t) {
    *n = *t = 0;
    struct dirent *entry;
    while (true) {
        entry = readdir(dir);
        if (entry == NULL) break;
        char *name = entry->d_name;
        if (! valid(name)) continue;
        *n = *n + 1;
        *t = *t + strlen(name) + 2;
    }
}

// Gather names from a directory, leaving room for adding slashes.
static void gatherNames(DIR *dir, int n, char *names[n], int t, char text[t]) {
    int i = 0, length = 0;
    struct dirent *entry;
    while (true) {
        entry = readdir(dir);
        if (entry == NULL) break;
        char *name = entry->d_name;
        if (! valid(name)) continue;
        char *file = &text[length];
        names[i++] = file;
        strcpy(file, name);
        length = length + strlen(name);
        text[++length] = '\0';
        length++;
    }
}

// Check whether a given entry in a given directory is a subdirectory.
static bool isDir(char const *dir, char *name) {
    struct stat info;
    char path[strlen(dir) + strlen(name) + 1];
    strcpy(path, dir);
    strcat(path, name);
    stat(path, &info);
    return S_ISDIR(info.st_mode);
}

static char *readDirectory(char const *path) {
    assert(path[strlen(path) - 1] == '/');
    DIR *dir = opendir(path);
    if (dir == NULL) { err("can't read dir", path); return NULL; }
    int count, size;
    measureDirectory(dir, &count, &size);
    rewinddir(dir);
    char **names = malloc(count * sizeof(char *));
    char *text = malloc(size);
    gatherNames(dir, count, names, size, text);
    closedir(dir);
    for (int i=0; i<count; i++) {
        if (isDir(path, names[i])) strcat(names[i], "/");
    }
    sort(count, names);
    char *result = malloc(size);
    result[0] = '\0';
    for (int i = 0; i < count; i++) {
        strcat(result, names[i]);
        strcat(result, "\n");
    }
    free(names);
    free(text);
    return result;
}

char *readPath(char const *path) {
    if (path[strlen(path) - 1] == '/') return readDirectory(path);
    else return readFile(path);
}

void writeFile(char const *path, int size, char data[size]) {
    assert(path[strlen(path) - 1] != '/');
    FILE *file = fopen(path, "wb");
    if (file == NULL) { err("can't write", path); return; }
    fwrite(data, size, 1, file);
    fclose(file);
}

#ifdef test_file

// Unit testing.
int main(int n, char *args[n]) {
    findResources(args[0]);
    char *snipe = current + strlen(current) - 6;
    assert(strncmp(snipe, "snipe", 5) == 0);
    assert(! absolute(""));
    assert(! absolute("prog.xxx"));
    assert(! absolute("./prog"));
    assert(absolute("/d/prog"));
    assert(absolute("c:/d/prog"));
    strcpy(current, "/a/b/");
    findInstall("/a/b/");
    assert(strcmp(install, "/a/b/") == 0);
    findInstall("/a/b/w");
    assert(strcmp(install, "/a/b/") == 0);
    findInstall("prog");
    assert(strcmp(install, "/a/b/") == 0);
    findInstall("./prog");
    assert(strcmp(install, "/a/b/") == 0);
    char *file = "c.txt";
    char *s = addPath("/a/b/", file);
    assert(strcmp(s, "/a/b/c.txt") == 0);
    free(s);
    assert(compare("", "") == 0);
    assert(compare("abcxaaaa", "abcyaaaa") < 0);
    assert(compare("abc", "abcx") < 0);
    assert(compare("abcx", "abc") > 0);
    assert(compare("abc100x", "abc9x") > 0);
    assert(compare("abc9x", "abc10x") < 0);
    assert(compare("abc9", "abc10") < 0);
    assert(compare("abc9def", "abc09defx") < 0);
    assert(compare("abc09def", "abc9defx") < 0);
    char *ss[4] = { "abc10", "abc9", "abc", ".." };
    sort(4, ss);
    assert(strcmp(ss[0], "..") == 0);
    assert(strcmp(ss[1], "abc") == 0);
    assert(strcmp(ss[2], "abc9") == 0);
    assert(strcmp(ss[3], "abc10") == 0);
    char *text = readPath("freetype/");
    assert(strncmp(text, "../\nMakefile", 12) == 0);
    free(text);
    freeResources();
    printf("File module OK\n");
    return 0;
}

#endif
