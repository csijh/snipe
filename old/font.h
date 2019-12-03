// The Snipe editor is free and open source, see licence.txt.
#include <stdint.h>

// A font holds the data for generating character glyphs at any size, plus
// cached pages of generated data. Font files are in ttf format and are handled
// via the Freetype library.
struct font;
typedef struct font font;

// A page contains information for a block of 256 characters from a font at a
// given size. The start character is a multiple of 256. The height and width
// give the size of a strip image containing the glyph for each character,
// expanded so they all have the same width, and the same ascent and descent and
// therefore height. The shared ascent, i.e. the number of pixels above the
// baseline, can be used to combine characters from different pages. (Even a
// monospaced font is not truly monospaced, particularly between pages.) The
// width of each character is width/256. The advances array gives the number of
// pixels to move on after drawing a character, typically a little less than the
// width. In the image, each pixel consists of 4 RGBA bytes, representing white
// with the font data in the transparency byte, so that arbitrary foreground and
// background colours can be applied. The coordinates are (y,x) with y upwards,
// i.e. the first row of pixels is the bottom edge of the characters.
struct page;
typedef struct page page;

// Create a new font object, from the file specified.
font *newFont(char *file);

// Free the font object and its cached pages.
void freeFont(font *f);

// Find or make a page for a font, size and start character (multiple of 256).
page *getPage(font *f, int size, int start);

// Free up a page, removing it from the chain of cached pages.
void freePage(page *p);

// Get the information from a page.
int pageWidth(page *p);
int pageHeight(page *p);
unsigned char *pageImage(page *p);
int charAdvance(page *p, int ch);
