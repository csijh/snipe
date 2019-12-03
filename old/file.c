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
#include "list.h"
#include "unicode.h"
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
#include <sys/types.h>

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

// Find the installation directory from args[0]. Free the previous value, so
// that the function can be called multiple times for testing. If in the src
// directory, use the parent.
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
    int n5 = strlen(install) - 5;
    if (strcmp(install + n5, "/src/") == 0) {
        install[n5 + 1] = '\0';
    }
}

void findResources(char const *program) {
    findCurrent();
    findInstall(program);
}

void freeResources() {
    free(current);
    free(install);
}

// Join two strings.
static char *join(char const *s1, char const *s2) {
    int len = strlen(s1) + strlen(s2) + 1;
    char *s = malloc(len);
    strcpy(s, s1);
    strcat(s, s2);
    return s;
}

char *parentPath(char const *path) {
    int n = strlen(path);
    if (n > 0 && path[n - 1] == '/') n--;
    while (n > 0 && path[n - 1] != '/') n--;
    char *s = malloc(n + 1);
    strncpy(s, path, n);
    s[n] = '\0';
    return s;
}

char *addPath(char const *path, char const *file) {
    if (strcmp(file, "..") == 0) return parentPath(path);
    if (strcmp(file, "../") == 0) return parentPath(path);
    if (strcmp(file, ".") == 0) file = "";
    else if (strcmp(file, "./") == 0) file = "";
    else if (absolute(file)) path = "";
    return join(path, file);
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

// Check if a path represents a directory.
static bool isDirPath(const char *path) {
    struct stat info;
    stat(path, &info);
    return S_ISDIR(info.st_mode);
}

char *fullPath(char const *file) {
    if (current == NULL) crash("Must call findResources first");
    char *path = addPath(current, file);
    if (isDirPath(path) && path[strlen(path) - 1] != '/') {
        char *p = path;
        path = join(path, "/");
        free(p);
    }
    for (int i = 0; i < strlen(path); i++) if (path[i] == '\\') path[i] = '/';
    return path;
}

char *extension(char const *path) {
    int n = strlen(path);
    if (n == 0) return "txt";
    if (path[n-1] == '/') return "directory";
    if (strcmp(&path[n-8], "Makefile") == 0) return "makefile";
    if (strcmp(&path[n-8], "makefile") == 0) return "makefile";
    char *ext = strrchr(path, '.');
    if (ext == NULL) return "txt";
    char *slash = strrchr(path, '/');
    if (ext < slash) return "txt";
    return ext + 1;
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
    if (n > 0 && data[n - 1] != '\n') data[n++] = '\n';
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

#ifndef _WIN32

// Read directory entries into an array of names. Represent the strings as
// indexes, in case the underlying character array moves. Leave space to add a
// slash on the end of subdirectory names.
static void readEntries(char const *path, ints *names, chars *text) {
    DIR *dir = opendir(path);
    if (dir == NULL) { err("can't read dir", path); return; }
    struct dirent *entry;
    for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
        char *name = entry->d_name;
        if (! valid(name)) continue;
        int index = length(text);
        resize(text, index + strlen(name) + 2);
        strcpy(&C(text)[index], name);
        int n = length(names);
        resize(names, n + 1);
        I(names)[n] = index;
    }
    closedir(dir);
}

#else

// For Windows, use the native UTF16 functions and convert to/from UTF8.
static void readEntries(char const *path, ints *names, chars *text) {
    wchar_t wpath[2 * strlen(path)];
    utf8to16(path, wpath);
    _WDIR *dir = _wopendir(wpath);
    if (dir == NULL) { err("can't read dir", path); return; }
    struct _wdirent *entry;
    for (entry = _wreaddir(dir); entry != NULL; entry = _wreaddir(dir)) {
        wchar_t *wname = entry->d_name;
        char name[2 * wcslen(wname)];
        utf16to8(wname, name);
        if (! valid(name)) continue;
        int index = length(text);
        resize(text, index + strlen(name) + 2);
        strcpy(&C(text)[index], name);
        int n = length(names);
        resize(names, n + 1);
        I(names)[n] = index;
    }
    _wclosedir(dir);
}

#endif

// Check whether a given entry in a given directory is a subdirectory.
static bool isDir(char const *dir, char *name) {
    char path[strlen(dir) + strlen(name) + 1];
    strcpy(path, dir);
    strcat(path, name);
    return isDirPath(path);
}

static char *readDirectory(char const *path) {
    assert(path[strlen(path) - 1] == '/');
    ints *inames = newInts();
    chars *text = newChars();
    resize(text, strlen(path) + 2);
    strcpy(C(text), path);
    resize(inames, 1);
    I(inames)[0] = 0;
    readEntries(path, inames, text);
    int count = length(inames);
    char **names = malloc(count * sizeof(char *));
    for (int i = 0; i < count; i++) names[i] = &C(text)[I(inames)[i]];
    freeList(inames);
    for (int i=1; i<count; i++) {
        if (isDir(path, names[i])) strcat(names[i], "/");
    }
    sort(count - 1, &names[1]);
    char *result = malloc(length(text));
    result[0] = '\0';
    for (int i = 0; i < count; i++) {
        strcat(result, names[i]);
        strcat(result, "\n");
    }
    free(names);
    freeList(text);
    return result;
}

char *readPath(char const *path) {
    if (path[strlen(path) - 1] == '/') return readDirectory(path);
    else return readFile(path);
}

// Write out a Makefile, restoring the tabs.
static void writeMakefile(FILE *file, int size, char data[size]) {
    int i = 0, j = 0;
    while (i < size) {
        if (data[j] == ' ') {
            while (data[j] == ' ') j++;
            fwrite("\t", 1, 1, file);
            i = j;
        }
        while (data[j] != '\n') j++;
        j++;
        fwrite(&data[i], j - i, 1, file);
        i = j;
    }
}

void writeFile(char const *path, int size, char data[size]) {
    assert(path[strlen(path) - 1] != '/');
    FILE *file = fopen(path, "wb");
    if (file == NULL) { err("can't write", path); return; }
    if (strcmp(&path[strlen(path) - 9], "/Makefile") == 0) {
        writeMakefile(file, size, data);
    }
    else fwrite(data, size, 1, file);
    fclose(file);
}

#ifdef fileTest
// Unit testing.

// Test the program is being tested from snipe/src.
static void testSnipe() {
    char *snipe = current + strlen(current) - 10;
    assert(strncmp(snipe, "snipe/src", 9) == 0);
}

static void testAbsolute() {
    assert(! absolute(""));
    assert(! absolute("prog.xxx"));
    assert(! absolute("./prog"));
    assert(absolute("/d/prog"));
    assert(absolute("c:/d/prog"));
}

static void testFindInstall() {
    strcpy(current, "/a/b/");
    findInstall("/a/b/");
    assert(strcmp(install, "/a/b/") == 0);
    findInstall("/a/b/w");
    assert(strcmp(install, "/a/b/") == 0);
    findInstall("prog");
    assert(strcmp(install, "/a/b/") == 0);
    findInstall("./prog");
    assert(strcmp(install, "/a/b/") == 0);
}

static void testAddPath() {
    char *s = addPath("/a/b/", "c.txt");
    assert(strcmp(s, "/a/b/c.txt") == 0);
    free(s);
    s = addPath("/a/b/", "/c.txt");
    assert(strcmp(s, "/c.txt") == 0);
    free(s);
    s = addPath("/a/b/", ".");
    assert(strcmp(s, "/a/b/") == 0);
    free(s);
    s = addPath("/a/b/", "..");
    assert(strcmp(s, "/a/") == 0);
    free(s);
}

static void testExtension() {
    assert(strcmp(extension("program.c"), "c") == 0);
    assert(strcmp(extension("/path/program.c"), "c") == 0);
    assert(strcmp(extension("/path.c/program"), "txt") == 0);
    assert(strcmp(extension("/path/"), "directory") == 0);
    assert(strcmp(extension("Makefile"), "makefile") == 0);
    assert(strcmp(extension("/path/makefile"), "makefile") == 0);
}

static void testCompare() {
    assert(compare("", "") == 0);
    assert(compare("abcxaaaa", "abcyaaaa") < 0);
    assert(compare("abc", "abcx") < 0);
    assert(compare("abcx", "abc") > 0);
    assert(compare("abc100x", "abc9x") > 0);
    assert(compare("abc9x", "abc10x") < 0);
    assert(compare("abc9", "abc10") < 0);
    assert(compare("abc9def", "abc09defx") < 0);
    assert(compare("abc09def", "abc9defx") < 0);
}

static void testSort() {
    char *ss[4] = { "abc10", "abc9", "abc", ".." };
    sort(4, ss);
    assert(strcmp(ss[0], "..") == 0);
    assert(strcmp(ss[1], "abc") == 0);
    assert(strcmp(ss[2], "abc9") == 0);
    assert(strcmp(ss[3], "abc10") == 0);
}

static void testReadDirectory() {
    char *text = readPath("../freetype/");
    free(text);
}

int main(int n, char *args[n]) {
    findResources(args[0]);
    testSnipe();
    testAbsolute();
    testFindInstall();
    testAddPath();
    testExtension();
    testCompare();
    testSort();
    testReadDirectory();
    freeResources();
    printf("File module OK\n");
    return 0;
}

#endif
