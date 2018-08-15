// The Snipe editor is free and open source, see licence.txt.

// TODO: Unicode characters
// TODO: font pages.
// TODO: Draw BAD with specific bg/fg colours. Draw highlight as a bg colour
// TODO: find scaling factor for high resolution monitors.
// TODO: save before quitting.

#include "display.h"
#include "event.h"
#include "action.h"
#include "style.h"
#include "font.h"
#include "theme.h"
#include "setting.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <GLFW/glfw3.h>

// A display object represents the main window, with layout measurements. The
// display is a viewport of size rows x cols onto a notional grid of characters
// representing the entire document. Pad is the number of pixels round the edge
// of the window. Scroll is the pixel scroll position, from the overall top of
// the text, and scrollTarget is the pixel position smooth scroll animation is
// aiming for. Event handling is delegated to a handler object.
struct display {
    handler *h;
    font *f;
    int fontSize;
    theme *t;
    GLFWwindow *gw;
    GLuint fontTextureId;
    int rows, cols, charWidth, charHeight, advance, pad, width, height;
    int caretRow, caretCol, scroll, scrollTarget;
    bool showCaret;
    page *p;
};

// Report a GLFW error.
static void GLFWError(int code, char const *message) {
    fprintf(stderr, "GLFW Error: %s\n", message);
    glfwTerminate();
    exit(1);
}

// Check for OpenGL errors. Call periodically.
static void checkGLError() {
    GLenum code = glGetError();
    if (code == GL_NO_ERROR) return;
    while (code != GL_NO_ERROR) {
        fprintf(stderr, "GL Error: %d\n", code);
        code = glGetError();
    }
    glfwTerminate();
    exit(1);
}

void quit(display *d) {
    glfwDestroyWindow(d->gw);
    glfwTerminate();
    freeDisplay(d);
    exit(0);
}

// Create an invisible new window of arbitrary size, and use it to initialize
// OpenGL. Set up the default camera (identity MODELVIEW matrix). Set up texture
// mapping and blending so that text can be drawn from a font texture. Allocate
// a textureId for the font.
static void initWindow(display *d) {
    glfwSetErrorCallback(GLFWError);
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    d->gw = glfwCreateWindow(100, 100, "Snipe", NULL, NULL);
    glfwMakeContextCurrent(d->gw);
    glfwSwapInterval(1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenTextures(1, &d->fontTextureId);
    checkGLError();
}

void setTitle(display *d, char const *path) {
    char title[strlen(path) + 6];
    int i = strlen(path) - 1;
    if (i > 0 && path[i-1] == '/') i--;
    while (i > 0 && path[i-1] != '/') i--;
    strcpy(title, "Snipe ");
    strcat(title, &path[i]);
    glfwSetWindowTitle(d->gw, title);
}

// Set or change the font size. Calculate the display metrics. Load the font
// image into a texture. Set up an orthogonal projection, with y reversed so 2D
// graphics (right,down) pixel coordinates can be used. Set the display size and
// make it visible, if it isn't already.
static void setFontSize(display *d) {
    d->p = getPage(d->f, d->fontSize, 0);
    d->charWidth = pageWidth(d->p) / 256;
    d->charHeight = pageHeight(d->p);
    d->width = d->pad + d->cols * d->charWidth + d->pad;
    d->height = d->rows * d->charHeight + d->pad;
    d->showCaret = true;
    d->caretRow = d->caretCol = 0;
    d->scroll = 0;
    glBindTexture(GL_TEXTURE_2D, d->fontTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, pageWidth(d->p), pageHeight(d->p), 0,
        GL_RGBA, GL_UNSIGNED_BYTE, pageImage(d->p)
    );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, d->width, d->height, 0, -1, 1);
    glViewport(0, 0, d->width, d->height);
    glfwSetWindowSize(d->gw, d->width, d->height);
    glfwShowWindow(d->gw);
    checkGLError();
}

static void paintBackground(display *d) {
    colour *bg = findColour(d->t, GAP);
    glClearColor(red(bg)/255.0, green(bg)/255.0, blue(bg)/255.0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLError();
}

// Create and initialize the editor display.
display *newDisplay(char const *path) {
    display *d = malloc(sizeof(*d));
    char *fontFile =  getSetting(Font);
    d->fontSize = atoi(getSetting(FontSize));
    d->f = newFont(fontFile);
    d->t = newTheme();
    d->rows = atoi(getSetting(Rows));
    d->cols = atoi(getSetting(Columns));
    d->pad = 4;
    d->scroll = 0;
    d->scrollTarget = 0;
    initWindow(d);
    setTitle(d, path);
    d->h = newHandler(d->gw);
    setBlinkRate(d->h, atof(getSetting(BlinkRate)));
    setFontSize(d);
    paintBackground(d);
    glfwSwapBuffers(d->gw);
    paintBackground(d);
    return d;
}

void freeDisplay(display *d) {
    glDeleteTextures(1, &d->fontTextureId);
    glfwTerminate();
    freeFont(d->f);
    freeTheme(d->t);
    freeHandler(d->h);
    free(d);
}

void bigger(display *d) {
    if (d->fontSize <= 35) d->fontSize++;
    setFontSize(d);
}

void smaller(display *d) {
    if (d->fontSize >= 5) d->fontSize--;
    setFontSize(d);
}

void cycleTheme(display *d) {
    nextTheme(d->t);
    paintBackground(d);
}

void setScrollTarget(display *d, int row) {
    d->scrollTarget = row * d->charHeight;
    if (d->scroll != d->scrollTarget) frameTick(d->h);
}

// Paint a given rectangle.
static void paintRect(colour *c, float x1, float y1, float x2, float y2) {
    glColor3f(red(c)/255.0, green(c)/255.0, blue(c)/255.0);
    glDisable(GL_TEXTURE_2D);
    glRectf(x1, y1, x2, y2);
    glEnable(GL_TEXTURE_2D);
    checkGLError();
}

// Draw a character, returning its advance value. If the character is selected,
// draw the selection background colour first. If the character is preceded by a
// caret, draw it afterwards.
static int drawChar(display *d, int ch, int row, int pos, char style) {
    int index = ch % 256;
    float du = 1 / 256.0, dv = 1.0;
    float u = index / 256.0, v = 0.0;
    GLfloat texPoints[] = { u,v+dv, u,v, u+du,v, u+du,v+dv };
    glTexCoordPointer(2, GL_FLOAT, 0, texPoints);
    float x = d->pad + pos;
    float y = (d->charHeight) * row - d->scroll;
    float dx = d->charWidth, dy = d->charHeight;
    if (hasStyleFlag(style, SELECT)) {
        paintRect(findColour(d->t, SELECT), x, y, x+dx, y+dy);
    }
    colour *fg = findColour(d->t, clearStyleFlags(style));
    glColor3f(red(fg)/255.0, green(fg)/255.0, blue(fg)/255.0);
    GLfloat points[] = { x,y, x,y+dy, x+dx,y+dy, x+dx,y };
    glVertexPointer(2, GL_FLOAT, 0, points);
    glDrawArrays(GL_QUADS, 0, 4);
    if (hasStyleFlag(style, POINT) && d->showCaret) {
        paintRect(findColour(d->t, POINT), x-1.0, y, x, y+dy);
    }
    return charAdvance(d->p, index);
}

// Draw a line.
void drawLine(display *d, int row, chars *line, chars *styles) {
    int n = length(line);
    int advance = 0;
    if (n > d->cols) n = d->cols;
    for (int i=0; i<n; i++) {
        advance += drawChar(d, C(line)[i], row, advance, C(styles)[i]);
    }
    checkGLError();
}

// Toggle whether the caret is displayed.
void blinkCaret(display *d) {
    d->showCaret = ! d->showCaret;
}

// Scroll the display by a small amount, according to the (+ve or -ve) speed.
// The total number of lines of text is given, to limit the scrolling.
void scrollBy(display *d, int speed, int total) {
    int pixels = 1;
    if (speed < 0) { pixels = -1; speed = -speed; }
    for (int i=0; i<speed-1; i++) pixels *= 2;
    d->scroll = d->scroll + pixels;
    if (d->scroll < 0) d->scroll = 0;
    int maxScroll = (total - d->rows) * d->charHeight;
    if (maxScroll < 0) maxScroll = 0;
    if (d->scroll > maxScroll) d->scroll = maxScroll;
}

int firstRow(display *d) {
    return d->scroll / d->charHeight;
}

int lastRow(display *d) {
    if (d->scroll % d->charHeight == 0) return firstRow(d) + d->rows - 1;
    return firstRow(d) + d->rows;
}

// Show the page. Draw the background ready for the next frame. If scrolling,
// generate an animation tick.
void showFrame(display *d) {
    glfwSwapBuffers(d->gw);
    paintBackground(d);
}

static void smoothScroll(display *d) {
    int diff = d->scrollTarget - d->scroll;
    diff = (diff >= 0) ? (diff + 9)/10 : (diff - 9)/10;
    d->scroll += diff;
    if (d->scroll != d->scrollTarget) frameTick(d->h);
}

// Round to nearest column.
static void charPosition(display *d, int x, int y, int *row, int *col) {
    *row = (y + d->scroll) / d->charHeight;
    *col = (x - d->pad + d->charWidth / 2) / d->charWidth;
}

event getEvent(display *d, int *r, int *c, char const **t) {
    int x, y;
    event e = getRawEvent(d->h, &x, &y, t);
    charPosition(d, x, y, r, c);
    return e;
}

void actOnDisplay(display *d, action a, char const *s) {
    switch (a) {
        case Bigger: bigger(d); break;
        case Smaller: smaller(d); break;
        case CycleTheme: cycleTheme(d); break;
        case Blink: blinkCaret(d); break;
        case Tick: smoothScroll(d); break;
        case Open: case Load: d->scroll = 0; break;
        case Paste: pasteEvent(d->h); break;
        case Cut: case Copy: clip(d->h, s); break;
        default: break;
    }
}

#ifdef test_display

// Redraw with text for testing.
static void testRedraw(display *d) {
    paintBackground(d);
    chars *line = newChars();
    chars *styles = newChars();
    char *lines[] =  { "Line one\n", "Line two\n", "Line three\n" };
    char K=KEY, G=GAP, I=ID, S=SIGN, Q=STRING, T=TYPE;
    char N = addStyleFlag(addStyleFlag(NUMBER, POINT), SELECT);
    char n = addStyleFlag(NUMBER, SELECT);
    char lineStyles[][12] = {
        {K,K,K,K,G,I,I,I,G},
        {S,S,S,S,G,Q,Q,Q,G},
        {T,T,T,T,G,N,n,n,n,n,G}
    };
    for (int i=0; i<3; i++) {
        resize(line, strlen(lines[i]));
        resize(styles, strlen(lines[i]));
        for (int j=0; j<strlen(lines[i]); j++) C(line)[j] = lines[i][j];
        for (int j=0; j<strlen(lines[i]); j++) C(styles)[j] = lineStyles[i][j];
        drawLine(d, i, line, styles);
    }
    showFrame(d);
    checkGLError();
    freeList(line);
    freeList(styles);
}

// Interactive testing.
int main(int n, char const *args[n]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    display *d = newDisplay("");
    printf("Check visually and test events interactively\n");
    while (1) {
        int r, c;
        char *t;
        event et = getEvent(d, &r, &c, &t);
        if (et != BLINK) { printEvent(et, r, c, t); printf("\n"); }
        if (et == BLINK) blinkCaret(d);
        else if (et == addEventFlag(C_, TEXT)) {
            if (t[0] == '+' || t[0] == '=') bigger(d);
            else if (t[0] == '-') smaller(d);
        }
        else if (et == addEventFlag(C_, ENTER)) cycleTheme(d);
        else if (et == QUIT) quit(d);
        testRedraw(d);
    }
    printf("Display module OK\n");
    return 0;
}

#endif
