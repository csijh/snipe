// Snipe graphics display. Free and open source. See licence.txt.

// A display structure deals with the graphics aspects of the editor window.
struct display;
typedef struct display display;

// Each text byte is accompanied by a style byte. A style consists of a
// foreground colour index in the bottom 5 bits (0x1F), a background colour in
// the next two bits (0x60) and a flag in the top bit (0x80) to indicate that
// the byte is preceded by a cursor.  In addition, a style value of 0xFF
// indicates a byte which continues the current grapheme. (There are at most 31
// foreground colours.) A grapheme is counted as a single column position.
typedef unsigned char style;

// Create a new display.
display *newDisplay();

// Dispose of the display.
void freeDisplay(display *d);

// Get a display handle, of a type which depends on the graphics library.
void *getHandle(display *d);

// Draw some text from its UTF8 bytes and their styles.
void drawPage(display *d, char *bytes, style *styles);
