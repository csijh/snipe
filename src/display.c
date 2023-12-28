// Snipe editor. Free and open source, see licence.txt.
#include "display.h"
#include "unicode.h"
#include "text.h"
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
// #include <allegro5/allegro_primitives.h>

// TODO:
// al_draw_glyph
// al_get_glyph_width
// (al_get_glyph_dimensions)
// al_get_glyph_advance
// al_hold_bitmap_drawing(true)    before draw page
// al_hold_bitmap_drawing(false)   after

// A cell records, for a grapheme drawn at a particular row-column position, the
// byte position in the line, the horizontal pixel position across the screen,
// and the last code point to be drawn at that position. This supports
// conversion from (x,y) pixel coordinates to (row,col) text positions, and
// moving the cursor or editing left or right by one 'character'. The only type
// of grapheme supported by Allegro's ttf addon appears to be where glyphs have
// a zero advance between them.
struct cell { int bytes, pixels, code; };
typedef struct cell Cell;

// A display object represents the main window, with layout measurements. The
// display is a viewport of size rows x cols onto a notional grid of characters
// representing the entire document. Pad is the number of pixels round the edge
// of the window. Scroll is the pixel scroll position, from the overall top of
// the text, and scrollTarget is the pixel position to aim for when smooth
// scrolling. Event handling is delegated to a handler object.

struct display {
//    handler *h;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_FONT *font;
    ALLEGRO_COLOR theme[16];
    int width, height;
    int rows, cols;
    int charWidth, lineHeight, pad;
    cell **grid;
    int lastCode, byteCol;
};

// Solarized colours, see https://ethanschoonover.com/solarized/
const unsigned int
    base03  = 0x002b36,
    base02  = 0x073642,
    base01  = 0x586e75,
    base00  = 0x657b83,
    base0   = 0x839496,
    base1   = 0x93a1a1,
    base2   = 0xeee8d5,
    base3   = 0xfdf6e3,
    yellow  = 0xb58900,
    orange  = 0xcb4b16,
    cyan    = 0x2aa198,
    blue    = 0x268bd2,
    red     = 0xdc322f,
    magenta = 0xd33682,
    violet  = 0x6c71c4,
    green   = 0x859900;

// Solarized-light theme. See // https://ethanschoonover.com/solarized/. Base3
// is the normal background, base2 highlighted background, base1 comments,
// base00 normal text, base01 emphasized text.
shade solLight[16] = {
    base3, base2, base1, base0, base00, base01, base02, base03, yellow, orange,
    cyan, blue, red, magenta, violet, green
};

// Solarized-dark theme. See // https://ethanschoonover.com/solarized/. Base03
// is the normal background, base02 highlighted background, base01 comments,
// base0 normal text, base1 emphasized text.
shade solDark[16] = {
    base03, base02, base01, base00, base0, base1, base2, base3, yellow, orange,
    cyan, blue, red, magenta, violet, green
};

// Give an error message and stop.
static void crash(char const *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

// Check result of a function which returns 'true' for success.
static void try(bool b, char const *fmt, ...) {
    if (b) return;
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

/*
Display *newDisplay() {
    if(! al_init()) crash("Failed to initialize Allegro.");
    al_init_font_addon();
    al_init_ttf_addon();
    ALLEGRO_DISPLAY *d = al_create_display(890,530);
    if (! d) crash("Failed to create display.");
    ALLEGRO_FONT *font = al_load_ttf_font("../fonts/NotoSansMono-Regular.ttf",18,0);
    if (! font) crash("Could not load font.");
    al_clear_to_color(al_map_rgb(0,0x2b,0x36));
    ALLEGRO_COLOR fg = al_map_rgb(0x83,0x94,0x96);

}
*/

// Set up an array of colours.
static void setTheme(Display *d, shade *theme) {
    for (int i = 0; i < 16; i++) {
        shade c = theme[i];
        d->theme[i] = al_map_rgb((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF);
    }
}

// Create a new (rows,cols) grid of cells, to record character positions.
static Cell **newGrid(Display *d) {
    int h = d->rows, w = d->cols;
    Cell **grid = malloc(h * sizeof(Cell *) + h * (w+1) * sizeof(cell));
    Cell *matrix = (Cell *) (grid + h);
    for (int r = 0; r < h; r++) grid[r] = &matrix[r * (w+1)];
    return grid;
}

// Create a new display, with allegro and its font and ttf addons.
Display *newDisplay() {
    Display *d = malloc(sizeof(Display));
    try(al_init(), "Failed to initialize Allegro.");
    al_init_font_addon();
    al_init_ttf_addon();
    char *fontFile1 = "../fonts/NotoSansMono-Regular.ttf";
    char *fontFile2 = "../fonts/NotoSansSymbols2-Regular.ttf";
    d->font = al_load_ttf_font(fontFile1, 18, 0);
    try(d->font != NULL, "failed to load '%s'", fontFile1);
    ALLEGRO_FONT *font2 = al_load_ttf_font(fontFile2, 18, 0);
    try(d->font != NULL, "Failed to load '%s'.", fontFile2);
    al_set_fallback_font(d->font, font2);
    d->charWidth = al_get_text_width(d->font, "n");
    d->lineHeight = al_get_font_line_height(d->font);
    d->rows = 24;
    d->cols = 80;
    d->pad = 4;
    d->width = d->pad + d->cols * d->charWidth + d->pad;
    d->height = d->rows * d->lineHeight;
    d->grid = newGrid(d);
    al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
    d->display = al_create_display(d->width, d->height);
    try(d->display != NULL, "Failed to create display.");
    setTheme(d, solLight);
//    d->h = newHandler(d->display);
    return d;
}

void freeDisplay(Display *d) {
//    freeHandler(d->h);
    free(d->grid);
    al_destroy_font(d->font);
    al_destroy_display(d->display);
    free(d);
}
void *getHandle(Display *d) {
    return d->display;
}

void clear(Display *d) {
    al_clear_to_color(d->theme[0]);
}

void frame(Display *d) {
    al_flip_display();
}

// Draw a filled rectangle (without using the primitives addon). Use a
// temporarily small clipping window.
static void drawRectangle(Display *d, int c, int x, int y, int w, int h) {
    al_set_clipping_rectangle(x, y, w, h);
    al_clear_to_color(d->theme[c]);
    al_reset_clipping_rectangle();
}

static void drawCaret(Display *d, int row, int col) {
    int y = row * d->lineHeight;
    int x = d->grid[row][col].pixels - 1;
    drawRectangle(d, Caret, x, y, 1, d->lineHeight);
}

static void drawGlyph(Display *d, int x, int y, int fg, int bg, int code) {
    if (bg != Normal) drawRectangle(d, bg, x, y, width, d->lineHeight);
    al_draw_glyph(d->font, d->theme[fg], x, y, code);
}


// Draw a glyph. Return true to indicate a column increase.
static bool drawGlyph(Display *d, int row, int col, char *s, char *k, int p) {
    Character ch = getUTF8(&s[p]);
    Cell *cell = &d->grid[row][col];
    int advance = al_get_glyph_advance(d->font, cell->code, ch.code);
    if (advance == 0) {

    }


    bool result = true;



    // if row>0 && advance==0, col = col -1.
    // if overwrite, don't update cell.


    cell *last = &d->grid[row][col-1];
    if (row == 0) this->bytes = 0;
    else this->bytes = last->bytes + last->length;
    int advance = -1;
    if (row > 0) advance =
    if (advance == 0) {
        result = false;
        last->bytes = this->bytes;
        last->length += ch.length;
        last->code = ch.code;
    }
    else {
        this->code = ch.code;
        this->length = ch.length;
        if (row == 0) this->pixels = d->pad;
        else this->pixels = last->pixels + advance;
    }
    int fg = k[i] & 0x1F;
    int bg = (k[i] >> 5) & 0x03;
    bool caret = ((k[i] >> 7) & 0x01) != 0;
    int x = this->pixels, y = row * d->lineHeight;
    if (caret) drawCaret(d, row, col);
    return result;
}


// When drawing a line, x is the pixel position across the screen, col is the
// cell position in the grid. and i is the byte position in the text.
void drawLine(Display *d, int row, int n, char *bytes, unsigned char *tags) {
    Character cp = getUTF8(&bytes[0]);
    int x = d->pad, y = row * d->lineHeight, col = 0, i;
    for (i = 0; bytes[i] != '\n'; ) {
        if (col >= d->cols) {
            // TODO: continuation markers.
            row++;
            x = d->pad;
            y = row * d->lineHeight;
            col = 0;
        }
        Character next = getUTF8(&bytes[i+cp.length]);
        int width = al_get_glyph_advance(d->font, cp.code, next.code);
        if (width < 3) printf("w %d %d to %d\n", width, cp.code, next.code);
        d->grid[row][col] = (cell) { .bytes = i, .pixels = x };
        int bg = (tags[i] >> 4) & 0x0F;
        int fg = tags[i] & 0x0F;
        if (bg != 0) {
            drawRectangle(d,bg,x,y,width,d->lineHeight);
        }
        al_draw_glyph(d->font, d->theme[fg], x, y, cp.code);
        oldFg = fg;
        col++;
        x = x + width;
        i = i + cp.length;
        cp = next;
    }
    d->grid[row][col] = (cell) { .bytes = i, .pixels = x };
    for (col++; col <= d->cols; col++) {
        d->grid[row][col] = d->grid[row][col-1];
    }
}

void drawPage(Display *d, char *bytes, style *tags) {
    clear(d);
    for (int r = 0; r < d->rows; r++) {
        drawLine(d, r, bytes, tags);
        while (*bytes != '\n' && *bytes != '\0') {
            bytes++;
            tags++;
        }
        if (*bytes == '\0') break;
        bytes++;
        tags++;
    }
    drawCaret(d, 3, 5);
    frame(d);
}

rowCol findPosition(Display *d, int x, int y) {
    int row = y / d->lineHeight, col = 0;
    for (int c = 1; c <= d->cols; c++) {
        if (x > d->grid[row][c].pixels) col = c;
    }
    return (rowCol) { row, col };
}

event nextEvent(Display *d) { return getNextEvent(d->h); }
char *eventText(Display *d) { return getEventText(d->h); }
int eventX(Display *d) { return getEventX(d->h); }
int eventY(Display *d) { return getEventY(d->h); }

#ifdef displayTest

// styles for bad tokens, selected text, continuation byte or combiner
enum { b = 0x2c, h = 0x14, c = 0xFF };

// Different fg and bg colours.
static char *line1 = "id(12,COM) = 'xyz' 12. high // note\n";
static unsigned char styles1[] =
    { 4,4,4,8,8,4,7,7,7,4,4,4,4,9,9,9,9,9,4,b,b,b,4,h,h,h,h,4,1,1,1,1,1,1,1,4 };
static token words[] = {
    {I,2},{S,1},{V,2},{S,1},{C,3},{S,1},{G,1},{S,1},
    {G,1},{Q,5},{G,1},{B,3},{G,1},{M,0},{I,4},{D,0},{G,1},{C,7},{N,1}};

// Eight 2-byte and four 3-byte characters
static char *line2 = "æ í ð ö þ ƶ ə β ᴈ ῷ ⁑ €\n";
static unsigned char styles2[] = {
    4,c,4,4,c,4,4,c,4,4,c,4,4,c,4,4,c,4,4,c,4,4,c,4,4,c,c,4,4,c,c,4,4,c,c,4,4,c,
c,4 };

// A two-byte e character, and an e followed by a two-byte combiner
static char *line3 = "Raphaël Raphaël\n";
static unsigned char styles3[] = {
    4,4,4,4,4,4,c,4,4,4,4,4,4,4,4,c,c,4,4 };

// Visible control characters.
static char *line4 = "␀␁␂␃␄␅␆␇␈␉␡\n";
static unsigned char styles4[] = {
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

int main() {
    Display *d = newDisplay();
    clear(d);
    drawLine(d,0,line1,styles1);
    drawLine(d,1,line2,styles2);
    drawLine(d,2,line3,styles3);
    drawLine(d,3,line4,styles4);
    drawCaret(d, 0, 0);
    frame(d);
//    drawPage(d, text, styles);
    al_rest(20);
    setTheme(d, solDark);
    clear(d);
    drawLine(d,0,line1,styles1);
    drawLine(d,1,line2,styles2);
    drawLine(d,2,line3,styles3);
    drawLine(d,3,line4,styles4);
    drawCaret(d, 0, 0);
    frame(d);
//    drawPage(d, text, styles);
    al_rest(3);
    freeDisplay(d);
    al_uninstall_system();
    return 0;
}

#endif
