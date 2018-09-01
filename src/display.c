// The Snipe editor is free and open source, see licence.txt.

// TODO: Unicode characters
// TODO: font pages.
// TODO: Draw BAD with specific bg/fg colours. Draw highlight as a bg colour.

#define _POSIX_C_SOURCE 200809L
#include "display.h"
#include "queue.h"
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
#include <pthread.h>
#include <GLFW/glfw3.h>

// A display object represents the main window, with layout measurements. The
// display is a viewport of size rows x cols onto a notional grid of characters
// representing the entire document. Pad is the number of pixels round the edge
// of the window. Scroll is the pixel scroll position, from the overall top of
// the text, and scrollTarget is the pixel position to aim for when smooth
// scrolling. Magnify is 1 except on Mac retina screens, where it is 2 (i.e.
// 2x2 real pixels for each virtual screen pixel. Event handling is delegated
// to a handler object (and thread).
struct display {
    handler *h;
    queue *q;
    font *f;
    int fontSize;
    theme *t;
    GLFWwindow *gw;
    GLuint fontTextureId;
    int rows, cols, charWidth, charHeight, advance, pad, width, height, magnify;
    int scroll, scrollTarget, docRows;
    bool showCaret, focused;
    int showSize;
    page *p;
    runFunction *run;
    void *runArg;
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
        fprintf(stderr, "GL Error: %x\n", code);
        code = glGetError();
    }
    glfwTerminate();
    exit(1);
}

// Arrange to call the run function, handing over the OpenGL context.
static void *runRun(void *vd) {
    display *d = (display *) vd;
    glfwMakeContextCurrent(d->gw);
    enqueue(d->q, FRAME, 0, 0, NULL);
    d->run(d->runArg);
    glfwMakeContextCurrent(NULL);
    return NULL;
}

void startGraphics(display *d, runFunction *run, void *arg) {
    d->run = run;
    d->runArg = arg;
    pthread_t runner, ticker;
    pthread_create(&runner, NULL, &runRun, d);
    pthread_create(&ticker, NULL, &tick, d->h);
    handle(d->h);
    pthread_join(runner, NULL);
    pthread_join(ticker, NULL);
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
    int width, height;
    glfwGetFramebufferSize(d->gw, &width, &height);
    d->magnify = width / 100;
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
    char title[strlen("Snipe ") + strlen(path) + 1];
    int i = strlen(path) - 1;
    if (i < 0) i = 0;
    if (i > 0 && path[i-1] == '/') i--;
    while (i > 0 && path[i-1] != '/') i--;
    strcpy(title, "Snipe ");
    strcat(title, &path[i]);
    glfwSetWindowTitle(d->gw, title);
}

// Get OpenGL ready for drawing, based on the display metrics. Set up an
// orthogonal projection, with y reversed so 2D graphics (right,down) pixel
// coordinates can be used.
static void setupGL(display *d) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, d->width, d->height, 0, -1, 1);
    glViewport(0, 0, d->width, d->height);
    resizeWindow(d->h, d->width/d->magnify, d->height/d->magnify);
    checkGLError();
}

// Set or change the size of the window as the result of a change of font size.
// Calculate the display metrics. Load the font image into a texture. Set up an
// orthogonal projection, with y reversed so 2D graphics (right,down) pixel
// coordinates can be used. Set the display size.
static void setSize(display *d) {
    int targetRow;
    if (d->scrollTarget != 0) targetRow = d->scrollTarget / d->charHeight;
    d->p = getPage(d->f, d->fontSize, 0);
    d->charWidth = pageWidth(d->p) / 256;
    d->charHeight = pageHeight(d->p);
    d->width = d->pad + d->cols * d->charWidth + d->pad;
    d->height = d->rows * d->charHeight + d->pad;
    d->showCaret = true;
    if (d->scrollTarget != 0) d->scrollTarget = targetRow * d->charHeight;
    d->scroll = d->scrollTarget;
    glBindTexture(GL_TEXTURE_2D, d->fontTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, pageWidth(d->p), pageHeight(d->p), 0,
        GL_RGBA, GL_UNSIGNED_BYTE, pageImage(d->p)
    );
    setupGL(d);
}

static void paintBackground(display *d) {
    colour *bg = findColour(d->t, GAP);
    glClearColor(red(bg)/255.0, green(bg)/255.0, blue(bg)/255.0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLError();
}

// Create and initialize the display. Release the OpenGL context so the runner
// thread can use it.
display *newDisplay(char const *path) {
    display *d = malloc(sizeof(*d));
    initWindow(d);
    setTitle(d, path);
    char *fontFile =  getSetting(Font);
    d->fontSize = atoi(getSetting(FontSize)) * d->magnify;
    d->f = newFont(fontFile);
    d->t = newTheme();
    d->rows = atoi(getSetting(Rows));
    d->cols = atoi(getSetting(Columns));
    d->pad = 4;
    d->scroll = 0;
    d->scrollTarget = 0;
    d->docRows = 10;
    d->showCaret = false;
    d->focused = true;
    d->showSize = 0;
    d->q = newQueue();
    double blinkRate = atof(getSetting(BlinkRate));
    d->h = newHandler(d->gw, d->q, blinkRate);
    setSize(d);
    paintBackground(d);
    glfwSwapBuffers(d->gw);
    paintBackground(d);
    glfwShowWindow(d->gw);
    glfwMakeContextCurrent(NULL);
    return d;
}

// Call from main (handler) thread. Assume runner has released OpenGL context.
void freeDisplay(display *d) {
    glfwMakeContextCurrent(d->gw);
    glfwDestroyWindow(d->gw);
    glDeleteTextures(1, &d->fontTextureId);
    glfwTerminate();
    freeFont(d->f);
    freeTheme(d->t);
    freeHandler(d->h);
    freeQueue(d->q);
    free(d);
}

int pageRows(display *d) {
    return d->rows;
}

void setDocRows(display *d, int rows) {
    d->docRows = rows;
}

static void bigger(display *d) {
    if (d->fontSize <= 35 * d->magnify) d->fontSize += d->magnify;
    setSize(d);
}

static void smaller(display *d) {
    if (d->fontSize >= 5 * d->magnify) d->fontSize -= d->magnify;
    setSize(d);
}

static void cycleTheme(display *d) {
    nextTheme(d->t);
    paintBackground(d);
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
void drawLine(display *d, int row, int n, char *line, char *styles) {
    bool longLine = n > d->cols + 1;
    int advance = 0;
    if (n > d->cols + 1) n = d->cols + 1;
    for (int i=0; i<n; i++) {
        char ch = line[i];
        char st = styles[i];
        if (longLine && i == d->cols) { ch = '\0'; st = BAD; }
        else if (ch == '\n') ch = ' ';
        advance += drawChar(d, ch, row, advance, st);
    }
    checkGLError();
}

// Draw the window size in the bottom right corner.
static void drawSize(display *d) {
    char line[d->cols], styles[d->cols];
    int k = snprintf(line, d->cols, "%dx%d", d->cols, d->rows);
    memmove(&line[d->cols - k], &line[0], k);
    for (int i = 0; i < d->cols - k; i++) line[i] = ' ';
    for (int i = 0; i < d->cols; i++) styles[i] = BAD;
    int row = d->rows - 1 + d->scroll / d->charHeight;
    drawLine(d, row, d->cols, line, styles);
}

// Toggle whether the caret is displayed. Check whether the focus has been lost.
static void blinkCaret(display *d) {
    if (! d->focused) d->showCaret = false;
    else d->showCaret = ! d->showCaret;
}

static void checkResize(display *d) {
    int width, height;
    glfwGetWindowSize(d->gw, &width, &height);
    width = width * d->magnify;
    height = height * d->magnify;
    int cols = (width - 2 * d->pad) / d->charWidth;
    int rows = (height - d->pad) / d->charHeight;
    d->width = width;
    d->height = height;
    if (cols != d->cols || rows != d->rows) {
        d->cols = cols;
        d->rows = rows;
    }
    setupGL(d);
    d->showSize = 5;
    drawSize(d);
}

int firstRow(display *d) {
    return d->scroll / d->charHeight;
}

int lastRow(display *d) {
    if (d->scroll % d->charHeight == 0) return firstRow(d) + d->rows - 1;
    return firstRow(d) + d->rows;
}

// Show the page. Draw the background ready for the next frame.
void showFrame(display *d) {
    if (d->showSize > 0) { drawSize(d); d->showSize--; }
    glfwSwapBuffers(d->gw);
    paintBackground(d);
}

/*
// XXX Do a small amount of scrolling, and generate another frame event.
static void smoothScroll(display *d) {
int diff = d->scrollTarget - d->scroll;
diff = (diff >= 0) ? (diff + 9)/10 : (diff - 9)/10;
d->scroll += diff;
if (d->scroll != d->scrollTarget) enqueue(d->q, FRAME, 0, 0, NULL);
}
*/
// Add the scroll amount (+ve or -ve) from a SCROLL event to the target. The
// units are tenths of a line height. At this stage, the target may not be a
// multiple of the line height, but it gets adjusted later.
static void addToTarget(display *d, int tenths) {
    int delta = tenths * d->charHeight / 10;
    d->scrollTarget -= delta;
    if (d->scrollTarget < 0) d->scrollTarget = 0;
    int maxScroll = (d->docRows - 10) * d->charHeight;
    if (d->scrollTarget > maxScroll) d->scrollTarget = maxScroll;
}

// Add the scroll amount to the target, then scroll by a small amount, larger if
// there is further to go. If the target is reached, adjust it to a multiple of
// the line height.
static void doScroll(display *d, int tenths) {
    addToTarget(d, tenths);
    int diff = d->scrollTarget - d->scroll;
    int inc = (diff >= 0) ? (diff + 9)/10 : (diff - 9)/10;
    if (inc == diff && (d->scrollTarget % d->charHeight != 0)) {
        if (inc < 0) d->scrollTarget -= d->scrollTarget % d->charHeight;
        else d->scrollTarget += d->charHeight - d->scrollTarget % d->charHeight;
    }
    d->scroll += inc;
    if (d->scroll != d->scrollTarget) {
        enqueue(d->q, SCROLL, 0, 0, NULL);
    }
}
/*
// XXX Deal with a scroll. Use two strategies. For mouse wheels, expect y = 10 or
// y = -10. Set a one-line target and start an animation to get there. For
// touchpads, expect variable amounts to arrive at roughly animation rates, so
// scroll straight to the given point (like holding and dragging the paper).
static void doScroll(display *d, int x, int y) {
int maxScroll = (d->docRows - 10) * d->charHeight;
if (y == 10) {
d->scrollTarget -= d->charHeight;
if (d->scrollTarget < 0) d->scrollTarget = 0;
smoothScroll(d);
}
else if (y == -10) {
d->scrollTarget += d->charHeight;
if (d->scrollTarget > maxScroll) d->scrollTarget = maxScroll;
smoothScroll(d);
}
else if (y > 0) {
d->scroll -= y;
if (d->scroll < 0) d->scroll = 0;
d->scrollTarget = d->scroll;
d->scrollTarget = d->scrollTarget - d->scrollTarget % d->charHeight;
}
else if (y < 0) {
d->scroll -=y;
if (d->scroll > maxScroll) d->scroll = maxScroll;
d->scrollTarget = d->scroll;
d->scrollTarget += d->charHeight - 1 - (d->scrollTarget - 1) % d->charHeight;
}
}
*/
static void doPageUp(display *d) {
    doScroll(d, d->rows * 10);
}

static void doPageDown(display *d) {
    doScroll(d, - (d->rows * 10));
}

// Scale up and round pixels to nearest document row/column.
void charPosition(display *d, int x, int y, int *row, int *col) {
    x = x * d->magnify;
    y = y * d->magnify;
    *row = (y + d->scroll) / d->charHeight;
    *col = (x - d->pad + d->charWidth / 2) / d->charWidth;
}

event getEvent(display *d, int *px, int *py, char const **pt) {
    event e = dequeue(d->q, px, py, pt);
    return e;
}

void actOnDisplay(display *d, action a, int x, int y, char const *s) {
    switch (a) {
        case Bigger: bigger(d); break;
        case Smaller: smaller(d); break;
        case CycleTheme: cycleTheme(d); break;
        case Blink: blinkCaret(d); break;
        //        case Frame: smoothScroll(d); break;
        case Scroll: doScroll(d, y); break;
        case PageUp: doPageUp(d); break;
        case PageDown: doPageDown(d); break;
        case Open: case Load: d->scroll = 0; break;
        case Paste: pasteEvent(d->h); break;
        case Cut: case Copy: clip(d->h, s); break;
        case Resize: checkResize(d); break;
        case Focus: d->focused = true; break;
        case Defocus: d->focused = false; break;
        default: break;
    }
}

#ifdef test_display

// Redraw with text for testing.
static void testRedraw(display *d) {
    paintBackground(d);
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
        drawLine(d, i, strlen(lines[i]), lines[i], lineStyles[i]);
    }
    showFrame(d);
    checkGLError();
}

// Represents the runner thread. Grab the OpenGL context.
static void *run(void *vd) {
    display *d = (display *) vd;
    event e = BLINK;
    int x, y;
    char const *t;
    while (e != QUIT) {
        e = getEvent(d, &x, &y, &t);
        if (e != BLINK) printEvent(e, x, y, t, "\n");
        if (e == BLINK) blinkCaret(d);
        else if (e == addEventFlag(C_, '+')) bigger(d);
        else if (e == addEventFlag(C_, '=')) bigger(d);
        else if (e == addEventFlag(C_, '-')) smaller(d);
        else if (e == addEventFlag(C_, ENTER)) cycleTheme(d);
        else if (e == RESIZE) checkResize(d);
        testRedraw(d);
    }
    return NULL;
}

// Interactive testing.
int main(int n, char const *args[n]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    printf("Check looks OK, blink, window resize, font size, cycle theme\n");
    printf("Check range of events printed\n");
    display *d = newDisplay("");
    startGraphics(d, &run, d);
    freeDisplay(d);
    freeResources();
    printf("Display module OK\n");
    return 0;
}

#endif
