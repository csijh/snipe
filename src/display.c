// Snipe graphics display. Free and open source. See licence.txt.
// TODO: allow for variable width characters (e.g. Asian).
// TODO: and carets (inserted bytes?, separate issue?)
// TODO: decide how to access text (get(+-), ~ auto)
#include "display.h"
#include "unicode.h"
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

// A cell records, for a character position in a line, the byte position of the
// character in the line, and its horizontal pixel position.
struct cell { unsigned short bytes, pixels; };
typedef struct cell cell;

struct display {
    ALLEGRO_DISPLAY *display;
    ALLEGRO_FONT *font;
    ALLEGRO_COLOR theme[16];
    int width, height;
    int rows, cols;
    int charWidth, lineHeight, pad;
    cell **grid;
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
shade soll[] = {
    base3, base2, base1, base0, base00, base01, base02, base03, yellow, orange,
    cyan, blue, red, magenta, violet, green,
};

// Solarized-dark theme. See // https://ethanschoonover.com/solarized/. Base03
// is the normal background, base02 highlighted background, base01 comments,
// base0 normal text, base1 emphasized text.
shade sold[] = {
    base03, base02, base01, base00, base0, base1, base2, base3, yellow, orange,
    cyan, blue, red, magenta, violet, green,
};
/*
// Check result of a function which must succeed.
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
*/
// Set up an array of colours.
static void setTheme(display *d, shade *theme) {
    for (int i = 0; i < 16; i++) {
        shade c = theme[i];
        d->theme[i] = al_map_rgb((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF);
    }
}

// Create a new (rows,cols) grid of cells, to record character positions.
static cell **newGrid(display *d) {
    int h = d->rows, w = d->cols;
    cell **grid = malloc(h * sizeof(cell *) + h * (w+1) * sizeof(cell));
    cell *matrix = (cell *) (grid + h);
    for (int r = 0; r < h; r++) grid[r] = &matrix[r * (w+1)];
    return grid;
}

// Create a new display, with font and ttf addons.
display *newDisplay() {
    display *d = malloc(sizeof(display));
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
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
    d->display = al_create_display(d->width, d->height);
    try(d->display != NULL, "Failed to create display.");
    setTheme(d, soll);
    return d;
}

void freeDisplay(display *d) {
    free(d->grid);
    al_destroy_font(d->font);
    al_destroy_display(d->display);
    free(d);
}

void *getHandle(display *d) {
    return d->display;
}

void clear(display *d) {
    al_clear_to_color(d->theme[0]);
}

void frame(display *d) {
    al_flip_display();
}

// Draw a filled rectangle, without using the primitives addon.
static void drawRectangle(display *d, int c, int x, int y, int w, int h) {
    al_set_clipping_rectangle(x, y, w, h);
    al_clear_to_color(d->theme[c]);
    al_set_clipping_rectangle(0, 0, d->width, d->height);
}

void drawCaret(display *d, int row, int col) {
    int y = row * d->lineHeight;
    int x = d->grid[row][col].pixels;
//    int x = d->pad + col * d->charWidth;
    drawRectangle(d,12,x,y,1,d->lineHeight);
//    frame(d);
}

// When drawing a line, x is the pixel position across the screen, col is the
// cell position in the grid. and i is the byte position in the text.
void drawLine(display *d, int row, char *bytes, unsigned char *tags) {
    character cp = getCode(&bytes[0]);
    int x = d->pad, y = row * d->lineHeight, col = 0, oldFg, i;
    for (i = 0; bytes[i] != '\n'; ) {
        if (col >= d->cols) {
            // TODO: continuation markers.
            row++;
            x = d->pad;
            y = row * d->lineHeight;
            col = 0;
        }
        character next = getCode(&bytes[i+cp.length]);
        int width = al_get_glyph_advance(d->font, cp.code, next.code);
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

void drawPage(display *d, char *bytes, style *tags) {
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

rowCol findPosition(display *d, int x, int y) {
    int row = y / d->lineHeight, col = 0;
    for (int c = 1; c <= d->cols; c++) {
        if (x > d->grid[row][c].pixels) col = c;
    }
    return (rowCol) { row, col };
}

#ifdef displayTest

// styles for bad tokens, selected text, continuation byte or combiner
enum { b = 0x2c, h = 0x14, c = 0xFF };

// Different fg and bg colours.
static char *line1 = "id(12,CON) = 'xyz' 12. high // note\n";
static unsigned char styles1[] =
    { 4,4,4,8,8,4,7,7,7,4,4,4,4,9,9,9,9,9,4,b,b,b,4,h,h,h,h,4,1,1,1,1,1,1,1,4 };

// Eight 2-byte and four 3-byte characters
static char *line2 = "æ í ð ö þ ƶ ə β ᴈ ῷ ⁑ €\n";
static unsigned char styles2[] = {
    4,c,4,4,c,4,4,c,4,4,c,4,4,c,4,4,c,4,4,c,4,4,c,4,4,c,c,4,4,c,c,4,4,c,c,4,4,c,c,4 };

// A two-byte e character, and an e followed by a two-byte combiner
static char *line3 = "Raphaël Raphaël\n";
static unsigned char styles3[] = {
    4,4,4,4,4,4,c,4,4,4,4,4,4,4,4,c,c,4,4 };

// Visible control characters.
static char *line4 = "␀␁␂␃␄␅␆␇␈␉␡\n";
static unsigned char styles4[] = {
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

int main() {
    display *d = newDisplay();
    clear(d);
    drawLine(d,0,line1,styles1);
    drawLine(d,1,line2,styles2);
    drawLine(d,2,line3,styles3);
    drawLine(d,3,line4,styles4);
    drawCaret(d, 0, 0);
    frame(d);
//    drawPage(d, text, styles);
    al_rest(20);
    setTheme(d, sold);
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
