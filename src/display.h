// Snipe graphics display. Free and open source. See licence.txt.

// A display structure deals with the graphics aspects of the editor window.
struct display;
typedef struct display display;

// A shade is a colour/color stored in RGB format.
typedef unsigned int shade;

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

// Switch to the next available theme.
void switchTheme(display *d);
