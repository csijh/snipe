// The Snipe editor is free and open source. See licence.txt.

// Find the path to the installation directory from args[0]. This appears to be
// the only simple cross-platform technique which doesn't involve making an
// installer. Also find the current working directory on startup.

// The best cross-platform approach to paths appears to be to use / as a
// separator exclusively. (Windows libraries accept them and always have. It is
// only Windows programs which don't.). Then \ and / are both banned from
// individual file or directory names.

// Directory handling requires some Posix functions from unistd.h, dirent.h and
// sys/stat.h. See http://pubs.opengroup.org/onlinepubs/9699919799/.
#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include "file.h"
#include "array.h"
#include "unicode.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

// Get the current working directory, with trailing /.
char *findCurrent() {
    char *current = newArray(sizeof(char));
    current = adjustTo(current, 100);
    while (getcwd(current, length(current)) == NULL) {
        current = adjustBy(current, 100);
    }
    int n = strlen(current);
    for (int i = 0; i < n; i++) if (current[i] == '\\') current[i] = '/';
    current = padBy(current, 1);
    if (current[n - 1] != '/') strcat(current, "/");
    current = adjustTo(current, strlen(current));
    return current;
}

// Check whether a path is absolute. Allow for a Windows drive letter prefix.
static bool absolute(char const *path) {
    int n = strlen(path);
    if (n >= 1 && path[0] == '/') return true;
    if (n >= 2 && path[1] == ':') return true;
    return false;
}

// If in the src directory, use the parent.
char *findInstall(char const *arg0, char const *current) {
    char *install = newArray(sizeof(char));
    int n = strlen(arg0) + 1;
    install = adjustTo(install, n);
    strcpy(install, arg0);
    for (int i = 0; i < n; i++) if (install[i] == '\\') install[i] = '/';
    if (! absolute(install)) {
        if (n >= 2 && install[0]=='.' && install[1]=='/') {
            memmove(install, install + 2, n - 2);
            n = n - 2;
        }
        int c = strlen(current);
        install = adjustBy(install, c);
        memmove(install + c, install, n);
        strncpy(install, current, c);
    }
    char *suffix = strrchr(install, '/');
    assert(suffix != NULL);
    suffix[1] = '\0';
    int n5 = strlen(install) - 5;
    if (strcmp(install + n5, "/src/") == 0) {
        install[n5 + 1] = '\0';
    }
    install = adjustTo(install, strlen(install));
    return install;
}

// Make a path from a format and some pieces. The first piece must be absolute.
// The format is used like fprintf, but producing an array. Free with freeArray.
char *makePath(char const *format, ...) {
    char *path = newArray(sizeof(char));
    va_list args;
    va_start(args, format);
    int n = vsnprintf(path, 0, format, args);
    va_end(args);
    path = adjustTo(path, n);
    path = padBy(path, 1);
    va_start(args, format);
    vsnprintf(path, n+1, format, args);
    va_end(args);
    return path;
}

char *parentPath(char const *path) {
    int n = strlen(path);
    if (n > 0 && path[n - 1] == '/') n--;
    while (n > 0 && path[n - 1] != '/') n--;
    char *s = newArray(sizeof(char));
    s = adjustTo(s, n);
    s = padBy(s, 1);
    strncpy(s, path, n);
    s[n] = '\0';
    return s;
}

char *extension(char const *path) {
    int n = strlen(path);
    if (n == 0) return ".txt";
    if (path[n-1] == '/') return ".directory";
    if (strcmp(&path[n-8], "Makefile") == 0) return ".makefile";
    if (strcmp(&path[n-8], "makefile") == 0) return ".makefile";
    char *ext = strrchr(path, '.');
    if (ext == NULL) return ".txt";
    char *slash = strrchr(path, '/');
    if (ext < slash) return ".txt";
    return ext;
}

// Check if a path represents a directory.
static bool isDirPath(const char *path) {
    struct stat info;
    stat(path, &info);
    return S_ISDIR(info.st_mode);
}

// Use binary mode, so that the number of bytes read equals the file size.
char *readFile(char const *path, char *content) {
    assert(path[strlen(path) - 1] != '/');
    FILE *file = fopen(path, "rb");
    if (file == NULL) return warn("can't read", path);
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    if (size > INT_MAX-2) return warn("file too big:", path);
    content = adjustTo(content, size);
    int n = fread(content, 1, size, file);
    if (n != size) return warn("read failed", path);
    if (n > 0 && content[n - 1] != '\n') {
        content = adjustBy(content, 1);
        content[n++] = '\n';
    }
    content = padBy(content, 1);
    content[n] = '\0';
    fclose(file);
    return content;
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

// Read directory entries into an array of names. Leave space to add a slash on
// the end of subdirectory names.
static char **readEntries(char const *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) return warn("can't read dir", path);
    char **names = newArray(sizeof(char *));
    struct dirent *entry;
    for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
        if (! valid(entry->d_name)) continue;
        char *name = malloc(strlen(entry->d_name) + 2);
        strcpy(name, entry->d_name);
        names = adjustBy(names, 1);
        names[length(names) - 1] = name;
    }
    closedir(dir);
    return names;
}

#else

// For Windows, use the native UTF16 functions and convert to/from UTF8.
static char **readEntries(char const *path) {
    wchar_t wpath[2 * strlen(path)];
    utf8to16(path, wpath);
    _WDIR *dir = _wopendir(wpath);
    if (dir == NULL) return warn("can't read dir", path);
    char **names = newArray(sizeof(char *));
    struct _wdirent *entry;
    for (entry = _wreaddir(dir); entry != NULL; entry = _wreaddir(dir)) {
        wchar_t *wname = entry->d_name;
        char name0[2 * wcslen(wname)];
        utf16to8(wname, name0);
        if (! valid(name0)) continue;
        char *name = malloc(strlen(name0) + 2);
        strcpy(name, name0);
        names = adjust(names, 1);
        names[length(names) - 1] = name;
    }
    _wclosedir(dir);
    return names;
}

#endif

// Check whether a given entry in a given directory is a subdirectory.
static bool isDir(char const *dir, char *name) {
    char path[strlen(dir) + strlen(name) + 1];
    strcpy(path, dir);
    strcat(path, name);
    return isDirPath(path);
}

char *readDirectory(char const *path, char *content) {
    assert(path[strlen(path) - 1] == '/');
    char **names = readEntries(path);
    if (names == NULL) return NULL;
    int count = length(names);
    for (int i = 0; i < count; i++) {
        if (isDir(path, names[i])) strcat(names[i], "/");
    }
    sort(count, names);
    int total = 0;
    for (int i = 0; i < count; i++) total += strlen(names[i]) + 1;
    content = adjustTo(content, total);
    content = padBy(content, 1);
    content[0] = '\0';
    for (int i = 0; i < count; i++) {
        strcat(content, names[i]);
        strcat(content, "\n");
    }
    for (int i = 0; i < count; i++) free(names[i]);
    freeArray(names);
    return content;
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
    if (file == NULL) { warn("can't write", path); return; }
    if (strcmp(&path[strlen(path) - 9], "/Makefile") == 0) {
        writeMakefile(file, size, data);
    }
    else fwrite(data, size, 1, file);
    fclose(file);
}

// ---------- Testing ----------------------------------------------------------
#ifdef fileTest

// Test that the program is in .../snipe/src/.
static void testSnipe(char *current) {
    char *snipe = current + strlen(current) - 10;
    assert(strncmp(snipe, "snipe/src/", 10) == 0);
}

static void testAbsolute() {
    assert(! absolute(""));
    assert(! absolute("prog.xxx"));
    assert(! absolute("./prog"));
    assert(absolute("/d/prog"));
    assert(absolute("c:/d/prog"));
}

static void testFindInstall(char *current) {
    strcpy(current, "/a/b/");
    char *install = findInstall("/a/b/", current);
    assert(strcmp(install, "/a/b/") == 0);
    freeArray(install);
    install = findInstall("/a/b/w", current);
    assert(strcmp(install, "/a/b/") == 0);
    freeArray(install);
    install = findInstall("prog", current);
    assert(strcmp(install, "/a/b/") == 0);
    freeArray(install);
    install = findInstall("./prog", current);
    assert(strcmp(install, "/a/b/") == 0);
    freeArray(install);
}

static void testMakePath() {
    char *s = makePath("%s%s%s%s", "/a/", "b/", "c", ".d");
    assert(strcmp(s, "/a/b/c.d") == 0);
    freeArray(s);
}

static void testExtension() {
    assert(strcmp(extension("program.c"), ".c") == 0);
    assert(strcmp(extension("/path/program.c"), ".c") == 0);
    assert(strcmp(extension("/path.c/program"), ".txt") == 0);
    assert(strcmp(extension("/path/"), ".directory") == 0);
    assert(strcmp(extension("Makefile"), ".makefile") == 0);
    assert(strcmp(extension("/path/makefile"), ".makefile") == 0);
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
    char *text = newArray(sizeof(char));
    text = readDirectory("./", text);
//    printf("%s", text);
    freeArray(text);
}

int main(int n, char *args[n]) {
    char *current = findCurrent();
    char *install = findInstall(args[0], current);
    testSnipe(current);
    testAbsolute();
    testFindInstall(current);
    testMakePath();
    testExtension();
    testCompare();
    testSort();
    testReadDirectory();
    freeArray(install);
    freeArray(current);
    printf("File module OK\n");
    return 0;
}

#endif
