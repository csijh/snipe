// The Snipe editor is free and open source, see licence.txt.
#include "document.h"
#include "action.h"
#include "scan.h"
#include "line.h"
#include "history.h"
#include "string.h"
#include "setting.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

// TODO: move scanner into text.
// TODO: add cut/copy/paste, including for multiple cursors.

// A document holds the path of a file or folder, its content, undo and redo
// lists, a scroll target, whether or not there have been any changes since the
// last load or save, a scanner, line and line-style buffers, and position/text
// data for a pending action.
struct document {
    char *path;
    text *content;
    history *undos, *redos;
    int scrollTarget;
    bool changed;
    scanner *sc;
    chars *line, *lineStyles;
    int pos;
    char *text;
};

static document *newEmptyDocument() {
    document *d = malloc(sizeof(document));
    scanner *sc = newScanner();
    *d = (document) {
        .path = NULL, .content = NULL, .undos = NULL, .redos = NULL,
        .changed = false, .sc = sc, .scrollTarget = 0,
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
    char *ext = strchr(path, '.');
    if (ext != NULL) changeLanguage(d->sc, ext + 1);
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
    freeChars(d->line);
    freeChars(d->lineStyles);
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

int getScrollTarget(document *d) {
    return d->scrollTarget;
}

chars *getLine(document *d, int row) {
    ints *lines = getLines(d->content);
    int p = startLine(lines, row);
    int n = lengthLine(lines, row);
    getText(d->content, p, n, d->line);
    return d->line;
}

chars *getStyle(document *d, int row) {
    ints *lines = getLines(d->content);
    chars *styles = getStyles(d->content);
    int unstyled = findRow(lines, length(styles));
    for (int r = unstyled; r <= row; r++) {
        int p = startLine(lines, r);
        int n = getWidth(d, r);
        getText(d->content, p, n, d->line);
        scan(d->sc, r, d->line, d->lineStyles);
        assert(length(styles) >= p);
        resize(styles, p);
        addList(styles, d->lineStyles);
    }
    int n = getWidth(d, row);
    int p = startLine(lines, row);
    sublist(styles, p, n, d->lineStyles);
    return d->lineStyles;
}

void addCursorFlags(document *d, int row, int n, chars *styles) {
    applyCursors(getCursors(d->content), row, styles);
}

static void cutLeft(document *d) {
    deleteAt(d->content);
    d->changed = true;
}

static void cutRight(document *d) { deleteAt(d->content); d->changed = true; }

static void doPageUp(document *d) {
    d->scrollTarget -= 30;
    if (d->scrollTarget < 0) d->scrollTarget = 0;
}

static void doPageDown(document *d) {
    d->scrollTarget += 30;
    if (d->scrollTarget > getHeight(d) - 10) d->scrollTarget = getHeight(d) - 10;
}

static void doLineUp(document *d) {
    d->scrollTarget -= 1;
    if (d->scrollTarget < 0) d->scrollTarget = 0;
}

static void doLineDown(document *d) {
    d->scrollTarget += 1;
    if (d->scrollTarget > getHeight(d) - 10) d->scrollTarget = getHeight(d) - 10;
}

static void doInsert(document *d) {
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
    char *path = fullPath("help/index.xhtml");
    strcpy(line, cmd);
    char *pc = strchr(line, '%');
    strcpy(pc, path);
    system(line);
}

void setData(document *d, int row, int col, char *t) {
    ints *lines = getLines(d->content);
    if (row > getHeight(d)) row = getHeight(d);
    int start = startLine(lines, row);
    int len = lengthLine(lines, row);
    if (col > len) col = len;
    d->pos = start + col;
    d->text = t;
}

// Get the styles up to date up to the maximum cursor position before dispatch.
// Return a flag to say whether the display should be redrawn.
void actOnDocument(document *d, action a) {
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
        case CutLeftChar: markLeftChar(cs); cutLeft(d); break;
        case CutRightChar: markRightChar(cs); cutRight(d); break;
        case CutLeftWord: markLeftWord(cs); cutLeft(d); break;
        case CutRightWord: markRightWord(cs); cutRight(d); break;
        case CutUpLine: markUpLine(cs); cutLeft(d); break;
        case CutDownLine: markDownLine(cs); cutRight(d); break;
        case CutStartLine: markStartLine(cs); cutLeft(d); break;
        case CutEndLine: markEndLine(cs); cutRight(d); break;
        case Insert: doInsert(d); break;
        case Newline: doNewline(d); break;
        case PageUp: doPageUp(d); break;
        case PageDown: doPageDown(d); break;
        case LineUp: doLineUp(d); break;
        case LineDown: doLineDown(d); break;
        case Help: doHelp(d); break;
        case Point: point(cs, d->pos); break;
        case Select: doSelect(cs, d->pos); break;
        case AddPoint: addPoint(cs, d->pos); break;
        case Save: save(d); break;
        case Quit: save(d); break;
        default: break;
    }
    mergeCursors(cs);
}

#ifdef test_document

int main(int n, char *args[n]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    document *d = newDocument("document.h");
    assert(getHeight(d) > 30);
    int len = getWidth(d, 0);
    chars *line = getLine(d, 0);
    char *t = "// The Snipe editor is free and open source, see licence.txt.\n";
    assert(match(line, 0, len, t));
    freeDocument(d);
    printf("Document module OK\n");
    return 0;
}

#endif
