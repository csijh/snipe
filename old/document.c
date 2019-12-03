// The Snipe editor is free and open source, see licence.txt.
#include "document.h"
#include "action.h"
#include "scan.h"
#include "indent.h"
#include "line.h"
#include "history.h"
#include "style.h"
#include "string.h"
#include "setting.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

// TODO: move scanner into text.

// A document holds the path of a file or folder, its content, undo and redo
// lists, a scroll target, whether or not there have been any changes since the
// last load or save, a scanner, line and line-style buffers, and position/text
// data for a pending action.
struct document {
    char *path;
    char *language;
    text *content;
    history *undos, *redos;
    bool changed;
    scanner *sc;
    chars *line, *lineStyles;
    int pos;
    char const *text;
};

static document *newEmptyDocument() {
    document *d = malloc(sizeof(document));
    scanner *sc = newScanner();
    *d = (document) {
        .path = NULL, .language = "txt", .content = NULL,
        .undos = NULL, .redos = NULL,
        .changed = false, .sc = sc,
        .line = newChars(), .lineStyles = newChars()
    };
    return d;
}

static void freeDocumentData(document *d) {
    if (d->path != NULL) free(d->path);
    if (d->content != NULL) freeText(d->content);
    if (d->undos != NULL) freeHistory(d->undos);
    if (d->redos != NULL) freeHistory(d->redos);
}

static void save(document *d) {
    if (d->path != NULL && d->changed) writeText(d->content, d->path);
}

static void load(document *d, char const *path) {
    save(d);
    freeDocumentData(d);
    d->content = readText(path);
    if (d->content == NULL) return;
    d->path = malloc(strlen(path) + 1);
    strcpy(d->path, path);
    d->language = extension(d->path);
    changeLanguage(d->sc, d->language);
    d->undos = newHistory();
    d->redos = newHistory();
    d->changed = false;
}

document *newDocument(char const *path) {
    document *d = newEmptyDocument();
    load(d, path);
    return d;
}

void freeDocument(document *d) {
    freeDocumentData(d);
    freeScanner(d->sc);
    freeList(d->line);
    freeList(d->lineStyles);
    free(d);
}

char const *getPath(document *d) { return d->path; }

bool isDirectory(document *d) {
    int n = strlen(d->path);
    return d->path[n - 1] == '/';
}

int getHeight(document *d) { return length(getLines(d->content)); }

int getWidth(document *d, int row) {
    return lengthLine(getLines(d->content), row);
}

// Increase the indent on a given line.
static void insertIndent(document *d, int row, int n) {
    ints *lines = getLines(d->content);
    chars *styles = getStyles(d->content);
    int p = startLine(lines, row);
    expand(styles, p, n);
    C(styles)[p] = addStyleFlag(GAP, START);
    for (int i = 1; i < n; i++) C(styles)[p + i] = GAP;
    char spaces[n + 1];
    for (int i = 0; i < n; i++) spaces[i] = ' ';
    spaces[n] = '\0';
    insertText(d->content, p, spaces);
    // Undo the invalidation
    resize(styles, endLine(lines, row));
}

// Reduce the indent on a given line.
static void deleteIndent(document *d, int row, int n) {
    ints *lines = getLines(d->content);
    chars *styles = getStyles(d->content);
    int p = startLine(lines, row);
    delete(styles, p, n);
    deleteText(d->content, p, n);
    // undo the invalidation
    resize(styles, endLine(lines, row));
}

// Get scanning, styles and indenting up to date, for the given line, before
// giving it away.
static void repairLine(document *d, int r) {
    ints *lines = getLines(d->content);
    chars *styles = getStyles(d->content);
    ints *indents = getIndents(d->content);
    int p = startLine(lines, r);
    int n = getWidth(d, r);
    getText(d->content, p, n, d->line);
    scan(d->sc, r, d->line, d->lineStyles);
    assert(length(styles) >= p);
    resize(styles, p + n);
    memcpy(&C(styles)[p], C(d->lineStyles), n);
    resize(indents, r+1);
    if (strcmp(d->language, "c") == 0 || strcmp(d->language, "h") == 0) {
        int runningIndent = 0;
        if (r > 0) runningIndent = I(indents)[r-1];
        int wanted = findIndent(&runningIndent, n, C(d->line),
        C(d->lineStyles)
        );
        I(indents)[r] = runningIndent;
        int actual = getIndent(n, C(d->line));
        if (wanted > actual) insertIndent(d, r, wanted - actual);
        if (wanted < actual) deleteIndent(d, r, actual - wanted);
    }
}

chars *getLine(document *d, int row) {
    ints *lines = getLines(d->content);
    chars *styles = getStyles(d->content);
    int unstyled = findRow(lines, length(styles));
    for (int r = unstyled; r <= row; r++) repairLine(d, r);
    int p = startLine(lines, row);
    int n = lengthLine(lines, row);
    getText(d->content, p, n, d->line);
    return d->line;
}

chars *getStyle(document *d, int row) {
    ints *lines = getLines(d->content);
    chars *styles = getStyles(d->content);
    ints *indents = getIndents(d->content);
    int unstyled = findRow(lines, length(styles));
    for (int r = unstyled; r <= row; r++) repairLine(d, r);
    int n = getWidth(d, row);
    int p = startLine(lines, row);
    resize(d->lineStyles, n);
    memcpy(C(d->lineStyles), &C(styles)[p], n);
    int runIndent = 0;
    if (row > 0) runIndent = I(indents)[row-1];
    int indent = findIndent(&runIndent, n, C(d->line), C(d->lineStyles));
    (void)indent;
    return d->lineStyles;
}

void addCursorFlags(document *d, int row, int n, chars *styles) {
    applyCursors(getCursors(d->content), row, styles);
}

static void cutLeft(document *d) {
    deleteAt(d->content);
    d->changed = true;
}

static void cutRight(document *d) {
    deleteAt(d->content);
    d->changed = true;
}

// Delete any selection before inserting.
static void doInsert(document *d) {
    cutLeft(d);
    insertAt(d->content, d->text);
    d->changed = true;
}

static void doNewline(document *d) {
    insertAt(d->content, "\n");
    d->changed = true;
}

static void doHelp(document *d) {
    char line[100];
    char *cmd = getSetting(HelpCommand);
    char *index = getSetting(HelpIndex);
    char *path = resourcePath("", index, "");
    strcpy(line, cmd);
    char *pc = strchr(line, '%');
    strcpy(pc, path);
    system(line);
    free(path);
}

// Load the filename selected in a directory listing.
static void doLoad(document *d) {
    cursors *cs = getCursors(d->content);
    ints *lines = getLines(d->content);
    int p = cursorAt(cs, 0);
    int r = findRow(lines, p);
    chars *line = getLine(d, r);
    int n2 = length(line) - 1;
    C(line)[n2] = '\0';
    char *path = addPath(d->path, C(line));
    load(d, path);
    free(path);
}

// Load the parent directory of the current file.
static void doOpen(document *d) {
    char *path = parentPath(d->path);
    load(d, path);
    free(path);
}

void setRowColData(document *d, int row, int col) {
    ints *lines = getLines(d->content);
    if (row > getHeight(d)) row = getHeight(d);
    int start = startLine(lines, row);
    int len = lengthLine(lines, row);
    if (col >= len) col = len - 1;
    d->pos = start + col;
}

void setTextData(document *d, char const *t) {
    d->text = t;
}

// Get the styles up to date up to the maximum cursor position before dispatch.
// Return a flag to say whether the display should be redrawn.
char const *actOnDocument(document *d, action a) {
    cursors *cs = getCursors(d->content);
    getStyle(d, maxRow(cs));
    switch (a) {
        case MoveLeftChar: moveLeftChar(cs); break;
        case MoveRightChar: moveRightChar(cs); break;
        case MoveLeftWord: moveLeftWord(cs); break;
        case MoveRightWord: moveRightWord(cs); break;
        case MoveUpLine: moveUpLine(cs); break;
        case MoveDownLine: moveDownLine(cs); break;
        case MoveStartLine: moveStartLine(cs); break;
        case MoveEndLine: moveEndLine(cs); break;
        case MarkLeftChar: markLeftChar(cs); break;
        case MarkRightChar: markRightChar(cs); break;
        case MarkLeftWord: markLeftWord(cs); break;
        case MarkRightWord: markRightWord(cs); break;
        case MarkUpLine: markUpLine(cs); break;
        case MarkDownLine: markDownLine(cs); break;
        case MarkStartLine: markStartLine(cs); break;
        case MarkEndLine: markEndLine(cs); break;
        case CutLeftChar: pMarkLeftChar(cs); cutLeft(d); break;
        case CutRightChar: pMarkRightChar(cs); cutRight(d); break;
        case CutLeftWord: markLeftWord(cs); cutLeft(d); break;
        case CutRightWord: markRightWord(cs); cutRight(d); break;
        case CutUpLine: markUpLine(cs); cutLeft(d); break;
        case CutDownLine: markDownLine(cs); cutRight(d); break;
        case CutStartLine: markStartLine(cs); cutLeft(d); break;
        case CutEndLine: markEndLine(cs); cutRight(d); break;
        case Insert: doInsert(d); break;
        case Newline: doNewline(d); break;
        case Help: doHelp(d); break;
        case Point: point(cs, d->pos); break;
        case Select: doSelect(cs, d->pos); break;
        case AddPoint: addPoint(cs, d->pos); break;
        case Copy: gatherText(d->content, d->line); break;
        case Cut: gatherText(d->content, d->line); cutLeft(d); break;
        case Load: doLoad(d); break;
        case Save: save(d); break;
        case Defocus: save(d); break;
        case Open: doOpen(d); break;
        case Quit: save(d); break;
        default: break;
    }
    mergeCursors(getCursors(d->content));
    return C(d->line);
}

#ifdef documentTest

int main(int n, char *args[n]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    document *d = newDocument("document.h");
    assert(getHeight(d) > 30);
    int len = getWidth(d, 0);
    chars *line = getLine(d, 0);
    char *t = "// The Snipe editor is free and open source, see licence.txt.\n";
    assert(strncmp(C(line), t, len) == 0);
    freeDocument(d);
    printf("Document module OK\n");
    return 0;
}

#endif
