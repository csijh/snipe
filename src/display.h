// Snipe editor. Free and open source, see licence.txt.
// TODO: allow for variable width characters (e.g. Asian).
// TODO: and carets (inserted bytes?, separate issue?)
// TODO: decide how to access text (get(+-), ~ auto)
// TODO: newDisplay(font structure, and theme structure).
// TODO: config file (use allegro?)
// TODO: fonts = sequence (synonyms?)
// TODO: theme = colours (synonyms?)
// TODO: syntax = map token types to fg(/bg)(@font)
// TODO: keymap = ?
// TODO: draw cursor separately (bg+fg)
//#include "handler.h"

// A display structure deals with the graphics aspects of the editor window.
typedef struct display Display;

// A shade is a colour stored in RGB format. A theme is an array of 16 shades,
// where the background shades are among the first 8.
typedef unsigned int shade;
typedef shade *theme;

// A token is a fragment of text with a style and a length.
typedef struct token { unsigned char style, length; } token;

// A style constant specifies how to draw a byte of text. The top 4 bits specify
// a background, and the lower four bits specify a foreground, each being an
// index into the current theme. Each style constant has a one-letter
// abbreviation as well as a full name. The comments refer to dark solarised.

//base03, base02, base01, base00, base0, base1, base2, base3,
//yellow, orange, cyan, blue, red, magenta, violet, green

enum style {
    G=0x00, GAP=G,          // base03, default background
    M=0x10, MARK=M,         // base02 background, add to selected bytes
    W=0x70, WARN=W,         // base3 inverted background (add to warnings)
    H=0x0C, HERE=H,         // cursor colour
    B=0x0C, BAD=B,          // red (optionally add WARN)
    C=0x02, COMMENTED=C,    // base01
    Q=0x0A, QUOTED=Q,       // cyan
    E=0x0B, ESCAPED=E,      // blue
    S=0x04, SIGN=S,         // base0
    V=0x0D, VALUE=V,        // magenta
    K=0x0F, KEYWORD=K,      // green
    T=0x0B, TYPE=T,         // blue
    R=0x09, RESERVED=R,     // orange
    I=0x04, ID=I,           // base0
    P=0x08, PROPERTY=P,     // yellow
    F=0x0E, FUNCTION=F,     // violet
    O=0x04, OP=O,           // base0
};

// A row/col pair. A row is a zero-based line number relative to the top of the
// display, and a col is a zero-based byte position within the text of the line.
struct rowCol { int r, c; };
typedef struct rowCol rowCol;

// Each text byte is accompanied by a style byte. A style consists of a
// foreground shade index in the bottom 5 bits (0x1F), a background shade in
// the next two bits (0x60) and a flag in the top bit (0x80) to indicate that
// the byte is preceded by a cursor. A foreground index of 31 indicates a
// continuation character, drawn in the same foreground shade on top of the
// previous character.
typedef unsigned char style;

Display *newDisplay();

void freeDisplay(Display *d);

// Get a display handle, of a type which depends on the graphics library.
void *getHandle(Display *d);

// Draw some text from its UTF8 bytes and their styles.
void drawPage(Display *d, char *bytes, style *styles);

void drawCaret(Display *d, int row, int col);

// Convert from (x,y) pixel coordinates to (row,col) document coordinates.
rowCol findPosition(Display *d, int x, int y);

// Switch to the next available theme.
void switchTheme(Display *d);

void setTitle(Display *d, char const *title);

int pageRows(Display *d);
int pageCols(Display *d);
int firstRow(Display *d);
int lastRow(Display *d);

// TODO: x, y, s
int getEvent(Display *d);

// TODO: internalize? rowCol type?
int charPosition(Display *d, int x, int y);

// Create an image of a line from its row number, text, and style info.
void drawLine(Display *d, int row, int n, char *line, unsigned char *styles);

// Make recent changes appear on screen, with a vertical sync delay.
void showFrame(Display *d);

// Carry out the given action, if relevant.
//void actOnDisplay(Display *d, action a, int x, int y, char const *s);
