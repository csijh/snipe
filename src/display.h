// Snipe graphics display. Free and open source. See licence.txt.
// TODO: newDisplay(font structure, and theme structure).
// TODO: config file (use allegro?)
// TODO: fonts = sequence (synonyms?)
// TODO: theme = colours (synonyms?)
// TODO: syntax = map token types to fg(/bg)(@font)
// TODO: keymap = ?

// A display structure deals with the graphics aspects of the editor window.
struct display;
typedef struct display display;

// A shade is a colour stored in RGB format. A theme is an array of 16 shades.
typedef unsigned int shade;
typedef shade *theme;

// A style is an index for a foreground colour, an index for a background
// colour, and an index for a font.
//struct style { unsigned char fg, bg, fi; };
//typedef struct style style;

// A row/col pair.
struct rowCol { int r, c; };
typedef struct rowCol rowCol;

// Each text byte is accompanied by a style byte. A style consists of a
// foreground shade index in the bottom 5 bits (0x1F), a background shade in
// the next two bits (0x60) and a flag in the top bit (0x80) to indicate that
// the byte is preceded by a cursor. A foreground index of 31 indicates a
// continuation character, drawn in the same foreground shade on top of the
// previous character.
typedef unsigned char style;

// Create a new display.
display *newDisplay();

// Dispose of the display.
void freeDisplay(display *d);

// Get a display handle, of a type which depends on the graphics library.
void *getHandle(display *d);

// Draw some text from its UTF8 bytes and their styles.
void drawPage(display *d, char *bytes, style *styles);

void drawCaret(display *d, int row, int col);

// Convert from (x,y) pixel coordinates to (row,col) document coordinates.
rowCol findPosition(display *d, int x, int y);

// Switch to the next available theme.
void switchTheme(display *d);
