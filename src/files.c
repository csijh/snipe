// Snipe file and directory handling. Free and open source. See licence.txt.
#include "files.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

// Uses args[0] to find the installation directory. This appears to be the only
// simple cross-platform technique which doesn't involve making an installer.

// Some conditional compilation is required for (a) UTF8 filenames on Windows
// and (b) standard directories for preferences.

// Some Posix headers are required:
// unistd.h    for getcwd, getuid, getpwuid
// dirent.h    for opendir, readdir, closedir
// pwd.h       for getpwuid
// sys/stat.h  for stat
// See http://pubs.opengroup.org/onlinepubs/9699919799/.

#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>

// Files object, holding the current working directory on startup, the
// installation directory, and the user's home and preferences directories.
struct files {
    char *current, *install, *home, *prefs;
};

// Give an error message and stop.
static void crash(char const *message, char const *s) {
    fprintf(stderr, "Bug: %s %s\n", message, s);
    exit(1);
}

// Get the current working directory with trailing / on the end.
static char *findCurrent() {
    size_t size = 24;
    char *current = malloc(size);
    while (getcwd(current, size) == NULL) {
        size += 32;
        current = realloc(current, size);
    }
    size_t n = strlen(current);
    for (int i = 0; i < n; i++) if (current[i] == '\\') current[i] = '/';
    if (size < n + 2) current = realloc(current, n + 2);
    if (current[n - 1] != '/') strcat(current, "/");
    return current;
}

// Check whether a path is absolute. Allow for a Windows drive letter prefix.
static bool absolute(char const *path) {
    size_t n = strlen(path);
    if (n >= 1 && path[0] == '/') return true;
    if (n >= 2 && path[1] == ':') return true;
    return false;
}

// Find the installation directory from args[0] which holds the path to the
// program being run and from the current working directory.
static char *findInstall(char const *args0, char const *current) {
    size_t n = strlen(args0) + 1;
    char *install = malloc(n);
    strcpy(install, args0);
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
    if (suffix == NULL) crash("no / in", install);
    suffix[1] = '\0';
    if (strcmp(suffix - 4, "/src/") == 0) suffix[-3] = '\0';
    return install;
}

// Find the user's home directory. Try USERPROFILE and HOMEDRIVE+HOMEPATH (which
// should cover Windows), HOME (which should cover Linux and command-line macOS)
// and getpwuid (which should cover GUI macOS).
static char *findHome() {
    char *home, *drive = "";
    home = getenv("HOME");
    if (home == NULL) home = getenv("USERPROFILE");
    if (home == NULL) {
        home = getenv("HOMEPATH");
        drive = getenv("HOMEDRIVE");
        if (drive == NULL) home = NULL;
        if (home == NULL) drive = "";
    }
    if (home == NULL) {
        struct passwd *pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    if (home == NULL) return NULL;
    int n = strlen(drive) + strlen(home) + 2;
    char *result = malloc(n);
    strcpy(result, drive);
    strcat(result, home);
    if (result[strlen(result)-1] != '/') strcat(result, "/");
    return result;
}

#if defined _WIN23
static char *prefsPath = "AppData/Roaming/";
#elif defined __APPLE__
static char *prefsPath = "Library/Preferences/";
#else
static char *prefsPath = ".config/";
#endif

static char *findPrefs(char *home) {
    char *s = malloc(strlen(home) + strlen(prefsPath) + 1);
    strcpy(s, home);
    strcat(s, prefsPath);
    return s;
}

files *newFiles(char const *args0) {
    files *fs = malloc(sizeof(files));
    fs->current = findCurrent();
    fs->install = findInstall(args0, fs->current);
    fs->home = findHome();
    fs->prefs = findPrefs(fs->home);
    return fs;
}

void freeFiles(files *fs) {
    free(fs->current);
    free(fs->install);
    free(fs->home);
    free(fs->prefs);
    free(fs);
}

char *installDir(files *fs) { return fs->install; }
char *currentDir(files *fs) { return fs->current; }
char *homeDir(files *fs) { return fs->home; }
char *prefsDir(files *fs) { return fs->prefs; }

char *join(int n, ...) {
    va_list list;
    int len = 2;
    va_start(list, n);
    for (int i = 0; i < n; i++) {
        char *p = va_arg(list, char *);
        int n = strlen(p);
        len += n;
    }
    va_end(list);
    char *s = malloc(len);
    strcpy(s, "");
    va_start(list, n);
    for (int i = 0; i < n; i++) {
        char *p = va_arg(list, char *);
        strcat(s, p);
    }
    va_end(list);
    return s;
}

// Check if a path represents a directory.
static bool isDirPath(const char *path) {
    struct stat info;
    stat(path, &info);
    return S_ISDIR(info.st_mode);
}

char *fullPath(files *fs, char const *file) {
    char *path = join(2, fs->current, file);
    if (isDirPath(path) && path[strlen(path) - 1] != '/') {
        strcat(path, "/");
    }
    for (int i = 0; i < strlen(path); i++) if (path[i] == '\\') path[i] = '/';
    return path;
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

// Find the size of a text file, or -1.
int fileSize(char const *path) {
    struct stat info;
    int result = stat(path, &info);
    if (result < 0) return -1;
    if (! S_ISREG(info.st_mode)) return -1;
    if (info.st_size >= INT_MAX) return -1;
    int size = (int) info.st_size;
    return size;
}

// Print error message for a particular file and continue program.
static void err(char *e, char const *p) { printf("Error, %s: %s\n", e, p); }

// Use binary mode, so that the number of bytes read equals the file size.
static char *readFile(char const *path) {
    if (path[strlen(path) - 1] == '/') crash("readFile on dir", path);
    int size = fileSize(path);
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

// Read directory entries, including an extra entry for the full path of the
// directory, producing a NULL-terminated array of names. Freeing the array also
// frees the space used for the text of the names. Each name has room to add /
// on the end for directories.
static char **readEntries(char const *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) { err("can't read dir", path); return NULL; }
    struct dirent *entry;
    char *text = malloc(strlen(path) + 2);
    strcpy(text, path);
    text[strlen(path) + 1] = '\0';
    int count = 1, length = strlen(path) + 2;
    for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
        char *name = entry->d_name;
        if (! valid(name)) continue;
        text = realloc(text, length + strlen(name) + 2);
        strcpy(&text[length], name);
        text[length + strlen(name) + 1] = '\0';
        length = length + strlen(name) + 2;
        count++;
    }
    closedir(dir);
    int extra = (count + 1) * sizeof(char *);
    text = realloc(text, length + extra);
    memmove(&text[extra], text, length);
    char **names = (char **)text;
    int n = extra;
    for (int i = 0; i < count; i++) {
        names[i] = &text[n];
        n = n + strlen(names[i]) + 2;
    }
    names[count] = NULL;
    return names;
}

#else

// For Windows, use the native UTF16 functions and convert to/from UTF8.
static void utf16to8(wchar_t const *ws, char *s) {
    int n = wcslen(ws);
    int out = 0;
    for (int i = 0; i < n; i++) {
        wchar_t wc = ws[i];
        if (wc < 0x80) {
            s[out++] = (char) wc;
        }
        else if (wc < 0x800) {
            s[out++] = (char) (0xc0 | (wc >> 6));
            s[out++] = (char) (0x80 | (wc & 0x3f));
        }
        else if (wc < 0xd800 || wc >= 0xe000) {
            s[out++] = (char) (0xe0 | (wc >> 12));
            s[out++] = (char) (0x80 | ((wc >> 6) & 0x3f));
            s[out++] = (char) (0x80 | (wc & 0x3f));
        }
        else {
            int ch = 0x10000 + (((wc & 0x3ff) << 10) | (ws[++i] & 0x3ff));
            s[out++] = (char) (0xf0 | (ch >> 18));
            s[out++] = (char) (0x80 | ((ch >> 12) & 0x3f));
            s[out++] = (char) (0x80 | ((ch >> 6) & 0x3f));
            s[out++] = (char) (0x80 | (ch & 0x3f));
        }
    }
    s[out] = '\0';
}

static int getUTF8(char const *t, int *plength) {
    int ch = t[0], len = 1;
    if ((ch & 0x80) == 0) { *plength = len; return ch; }
    else if ((ch & 0xE0) == 0xC0) { len = 2; ch = ch & 0x3F; }
    else if ((ch & 0xF0) == 0xE0) { len = 3; ch = ch & 0x1F; }
    else if ((ch & 0xF8) == 0xF0) { len = 4; ch = ch & 0x0F; }
    for (int i = 1; i < len; i++) ch = (ch << 6) | (t[i] & 0x3F);
    *plength = len;
    return ch;
}

static void utf8to16(char const *s, wchar_t *ws) {
    int out = 0;
    for (int i = 0; i < strlen(s); ) {
        int len;
        int ch = getUTF8(&s[i], &len);
        i += len;
        if (ch < 0x10000) ws[out++] = (wchar_t) ch;
        else {
            ch = ch - 0x10000;
            ws[out++] = 0xd800 | ((ch >> 10) & 0x3ff);
            ws[out++] = 0xdc00 | (ch & 0x3ff);
        }
    }
    ws[out] = 0;
}

static void readEntries(char const *path, ints *names, chars *text) {
    wchar_t wpath[2 * strlen(path)];
    utf8to16(path, wpath);
    _WDIR *dir = _wopendir(wpath);
    if (dir == NULL) { err("can't read dir", path); return; }
    struct _wdirent *entry;
    char *text = malloc(strlen(path) + 2);
    strcpy(text, path);
    text[strlen(path) + 1] = '\0';
    int count = 1, length = strlen(path) + 2;
    for (entry = _wreaddir(dir); entry != NULL; entry = _wreaddir(dir)) {
        wchar_t *wname = entry->d_name;
        char name[2 * wcslen(wname)];
        utf16to8(wname, name);
        if (! valid(name)) continue;
        text = realloc(text, length + strlen(name) + 2);
        strcpy(&text[length], name);
        text[length + strlen(name) + 1] = '\0';
        length = length + strlen(name) + 2;
        count++;
    }
    _wclosedir(dir);
    int extra = (count + 1) * sizeof(char *);
    text = realloc(text, length + extra);
    memmove(&text[extra], text, length);
    char **names = (char **)text;
    int n = extra;
    for (int i = 0; i < count; i++) {
        names[i] = &text[n];
        n = n + strlen(names[i]) + 2;
    }
    names[count] = NULL;
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

static char *readDirectory(char const *path) {
    if (path[strlen(path) - 1] != '/') crash("dir not ending /", path);
    char **names = readEntries(path);
    int count = 1;
    for (int i = 1; names[i] != NULL; i++) {
        if (isDir(path, names[i])) strcat(names[i], "/");
        count++;
    }
    sort(count - 1, &names[1]);
    int n = 1;
    for (int i = 0; i < count; i++) n = n + strlen(names[i]) + 1;
    char *result = malloc(n);
    result[0] = '\0';
    for (int i = 0; i < count; i++) {
        strcat(result, names[i]);
        strcat(result, "\n");
    }
    free(names);
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

// Use binary mode, to preserve line endings as \n.
void writeFile(char const *path, int size, char data[size]) {
    if (path[strlen(path) - 1] == '/') { err("can't write", path); return; }
    FILE *file = fopen(path, "wb");
    if (file == NULL) { err("can't write", path); return; }
    if (strcmp(&path[strlen(path) - 9], "/Makefile") == 0) {
        writeMakefile(file, size, data);
    }
    else if (strcmp(&path[strlen(path) - 9], "/makefile") == 0) {
        writeMakefile(file, size, data);
    }
    else fwrite(data, size, 1, file);
    fclose(file);
}

// Unit testing.

#ifdef filesTest
#include <assert.h>

static void testAbsolute() {
    assert(! absolute(""));
    assert(! absolute("prog.xxx"));
    assert(! absolute("./prog"));
    assert(absolute("/d/prog"));
    assert(absolute("c:/d/prog"));
}

static void testFindInstall(files *fs) {
    char *in = findInstall("/a/b/prog","/a/b/");
    assert(strcmp(in, "/a/b/") == 0);
    free(in);
    in = findInstall("/c/d/prog","/a/b/");
    assert(strcmp(in, "/c/d/") == 0);
    free(in);
    in = findInstall("prog","/a/b/");
    assert(strcmp(in, "/a/b/") == 0);
    free(in);
    in = findInstall("./prog","/a/b/");
    assert(strcmp(in, "/a/b/") == 0);
    free(in);
}

static void testJoin() {
    char *s = join(2, "/a/b/", "c.txt");
    printf("bp %s\n", s);
    assert(strcmp(s, "/a/b/c.txt") == 0);
    free(s);
    s = join(3, "/a/", "b/", "c.txt");
    assert(strcmp(s, "/a/b/c.txt") == 0);
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

static void testReadDirectory(char *f) {
    char *text = readPath(f);
    printf("RESULT OF READING CURRENT DIRECTORY\n");
    printf("%s", text);
    free(text);
}

int main(int n, char *args[n]) {
    files *fs = newFiles(args[0]);
    printf("INSTALL %s\n", fs->install);
    printf("CURRENT %s\n", fs->current);
    printf("HOME    %s\n", fs->home);
    printf("PREFS   %s\n", fs->prefs);
    testAbsolute();
    testFindInstall(fs);
    testJoin();
    testExtension();
    testCompare();
    testSort();
    testReadDirectory(fs->current);
    freeFiles(fs);
    printf("Files module OK\n");
    return 0;
}

#endif
