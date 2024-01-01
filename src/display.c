// Snipe editor. Free and open source, see licence.txt.
#include "display.h"
#include "unicode.h"
#include "text.h"
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// A cell records, for a grapheme drawn at a particular row-column position, the
// byte position in the line, and the horizontal pixel position across the
// screen. This supports conversion from (x,y) pixel coordinates to
// (row,col) text positions, and moving the cursor or editing left or right by
// one 'character'. The only type of grapheme supported by Allegro's ttf addon
// appears to be where glyphs have a zero advance between them.
struct cell { int bytes, pixels; };
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
    ALLEGRO_COLOR theme[Caret+1];
    int width, height;
    int rows, cols;
    int rowHeight, colWidth, pad;
    Cell **grid;
    int oldBg, oldFg, oldCode;
};

static void loadTheme(Display *d, char *path) {
    for (int k = 0; k < Caret; k++) d->theme[k].a = 0;
    ALLEGRO_CONFIG *cfg = al_load_config_file(path);
    ALLEGRO_CONFIG_ENTRY *entry;
    char const *key = al_get_first_config_entry(cfg, NULL, &entry);
    for ( ; key != NULL; key = al_get_next_config_entry(&entry)) {
        char const *value = al_get_config_value(cfg, NULL, key);
        Kind k = findKind(key);
        if (k > Caret) { printf("Bad theme key %s\n", key); continue; }
        if (value[0] == '#') {
            int col = (int) strtol(&value[1], NULL, 16);
            d->theme[k] = al_map_rgb((col>>16)&0xFF, (col>>8)&0xFF, col&0xFF);
        }
        else {
            Kind prev = findKind(value);
            if (k > Caret)  { printf("Bad theme value %s\n", value); continue; }
            d->theme[k] = d->theme[prev];
        }
    }
    for (int k = 0; k < Caret; k++) {
        if (d->theme[k].a != 0) continue;
        char *name = kindName(k);
        if (strlen(name) == 1) continue;
        printf("No theme entry for %s\n", name);
    }
    al_destroy_config(cfg);
}

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

// Create a new (rows,cols) grid of cells, to record character positions.
static Cell **newGrid(Display *d) {
    int h = d->rows, w = d->cols;
    Cell **grid = malloc(h * sizeof(Cell *) + h * (w+1) * sizeof(Cell));
    Cell *matrix = (Cell *) (grid + h);
    for (int r = 0; r < h; r++) grid[r] = &matrix[r * (w+1)];
    return grid;
}

// Create a new display, with fonts, theme and ttf addons.
Display *newDisplay() {
    Display *d = malloc(sizeof(Display));
    loadTheme(d, "../themes/solarized-dark.txt");
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
    d->colWidth = al_get_text_width(d->font, "n");
    d->rowHeight = al_get_font_line_height(d->font);
    d->rows = 24;
    d->cols = 80;
    d->pad = 4;
    d->width = d->pad + d->cols * d->colWidth + d->pad;
    d->height = d->rows * d->rowHeight;
    d->grid = newGrid(d);
    al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
    d->display = al_create_display(d->width, d->height);
    try(d->display != NULL, "Failed to create display.");
//    d->h = newHandler(d->display);
    return d;
}

void freeDisplay(Display *d) {
//    freeHandler(d->h);
    free(d->grid);
    al_destroy_font(d->font);
    al_destroy_display(d->display);
    free(d);
    al_uninstall_system();
}

void *getHandle(Display *d) {
    return d->display;
}

void clear(Display *d) {
    al_clear_to_color(d->theme[Ground]);
}

void frame(Display *d) {
    al_flip_display();
}

// Draw a filled rectangle (without using the primitives addon). Use a
// temporarily small clipping window.
static void drawRectangle(ALLEGRO_COLOR c, int x, int y, int w, int h) {
    al_set_clipping_rectangle(x, y, w, h);
    al_clear_to_color(c);
    al_reset_clipping_rectangle();
}

// Draw a caret before the glyph at (x,y).
static void drawCaret(Display *d, int x, int y) {
    drawRectangle(d->theme[Caret], x-1, y, 1, d->rowHeight);
}

// Draw a background from the glyph at (x,y) to the screen width.
static void drawBackground(Display *d, int bg, int x, int y) {
    drawRectangle(d->theme[bg], x, y, (d->width-x), d->rowHeight);
}

// Draw a glyph given its Unicode code point, with possible preceding caret and
// change of background. Change the background back to the default after \n.
static void drawGlyph(Display *d, int x, int y, int code, int style) {
    int fg = foreground(style), bg = background(style);
    bool caret = hasCaret(style);
    if (fg == More) { fg = d->oldFg; bg = d->oldBg; }
    if (caret) drawCaret(d, x, y);
    if (bg != d->oldBg) drawBackground(d, bg, x, y);
    if (code != '\n') al_draw_glyph(d->font, d->theme[fg], x, y, code);
    else if (bg != Ground) drawBackground(d, Ground, x + d->colWidth, y);
}

// When drawing a row, x is the pixel position across the screen, col is the
// cell position in the grid. and i is the byte position in the text.
void drawRow(Display *d, int row, char *bytes, Style *styles) {
    d->oldCode = ALLEGRO_NO_KERNING;
    int x = d->pad, y = row * d->rowHeight, col = 0, i;
    for (i = 0; i==0 || bytes[i-1] != '\n'; ) {
        Character cp = getUTF8(&bytes[i]);
        int advance = al_get_glyph_advance(d->font, d->oldCode, cp.code);
        if (advance < 3) printf("%d %d %d\n", row, i, advance);
        x = x + advance;
        drawGlyph(d, x, y, cp.code, styles[i]);
        i = i + cp.length;
        d->oldCode = cp.code;
    }
    // if advance > 0, inc col and fill cell
}

/*
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


    Cell *last = &d->grid[row][col-1];
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
    int x = this->pixels, y = row * d->rowHeight;
    if (caret) drawCaret(d, row, col);
    return result;
}
*/
/*
// When drawing a line, x is the pixel position across the screen, col is the
// cell position in the grid. and i is the byte position in the text.
void drawLine(Display *d, int row, int n, char *bytes, unsigned char *styles) {
    Character cp = getUTF8(&bytes[0]);
    int x = d->pad, y = row * d->rowHeight, col = 0, i;
    for (i = 0; bytes[i] != '\n'; ) {
        if (col >= d->cols) {
            // TODO: continuation markers.
            row++;
            x = d->pad;
            y = row * d->rowHeight;
            col = 0;
        }
        Character next = getUTF8(&bytes[i+cp.length]);
        int width = al_get_glyph_advance(d->font, cp.code, next.code);
        if (width < 3) printf("w %d %d to %d\n", width, cp.code, next.code);
        d->grid[row][col] = (Cell) { .bytes = i, .pixels = x };
        int bg = (styles[i] >> 4) & 0x0F;
        int fg = styles[i] & 0x0F;
        if (bg != 0) {
            drawRectangle(d,bg,x,y,width,d->rowHeight);
        }
        al_draw_glyph(d->font, d->theme[fg], x, y, cp.code);
        oldFg = fg;
        col++;
        x = x + width;
        i = i + cp.length;
        cp = next;
    }
    d->grid[row][col] = (Cell) { .bytes = i, .pixels = x };
    for (col++; col <= d->cols; col++) {
        d->grid[row][col] = d->grid[row][col-1];
    }
}

void drawPage(Display *d, char *bytes, style *styles) {
    clear(d);
    for (int r = 0; r < d->rows; r++) {
        drawLine(d, r, bytes, styles);
        while (*bytes != '\n' && *bytes != '\0') {
            bytes++;
            styles++;
        }
        if (*bytes == '\0') break;
        bytes++;
        styles++;
    }
    drawCaret(d, 3, 5);
    frame(d);
}

rowCol findPosition(Display *d, int x, int y) {
    int row = y / d->rowHeight, col = 0;
    for (int c = 1; c <= d->cols; c++) {
        if (x > d->grid[row][c].pixels) col = c;
    }
    return (rowCol) { row, col };
}
*/
//event nextEvent(Display *d) { return getNextEvent(d->h); }
//char *eventText(Display *d) { return getEventText(d->h); }
//int eventX(Display *d) { return getEventX(d->h); }
//int eventY(Display *d) { return getEventY(d->h); }

#ifdef displayTest

// Convert character to style, for testing.
static Style convert(char ch) {
    switch (ch) {
    case 'I': return Identifier; case 'R': return Round; case 'V': return Value;
    case 'M': return Mark; case 'G': return Gap; case 'Q': return Quote;
    case 'C': return Comment;
    case 'v': return Value + ((Warn - Ground) << 5);
    case 'i': return Identifier + ((Select - Ground) << 5);
    default: return Identifier;
    }
}

// Abbreviated styles for testing
enum {I=Identifier, R=Round, V=Value, M=Mark, G=Gap, Q=Quote, C=Comment};
enum { v = Value + ((Warn - Ground) << 5) };
enum { i = Identifier + ((Select - Ground) << 5) };

// Different fg and bg colours.
static char *line1 =   "id(12,COM) = 'xyz' 12. high // note\n";
static char *styles1 = "IIRVVMVVVMGMGQQQQQGvvvGiiiiGCCCCCCCG";

// Eight 2-byte and four 3-byte characters
static char *line2 =   "æ í ð ö þ ƶ ə β ᴈ ῷ ⁑ €\n";
static char *styles2 = "IIGIIGIIGIIGIIGIIGIIGIIGIIIGIIIGIIIGIIIG";

// A two-byte e character, and an e followed by a two-byte combiner
static char *line3 =   "Raphaël Raphaël\n";
static char *styles3 = "IIIIIIIIGIIIIIIIIIG";

static void drawLine(Display *d, int row, char *line, char *styles) {
    Style s[strlen(styles) + 1];
    for (int i = 0; i < strlen(styles); i++) s[i] = convert(styles[i]);
        drawRow(d, row, line, s);
}

int main() {
    Display *d = newDisplay();
    clear(d);
    drawLine(d, 0, line1, styles1);
    drawLine(d, 1, line2, styles2);
    drawLine(d, 2, line3, styles3);
    frame(d);
    al_rest(10);
    freeDisplay(d);
    return 0;
}

#endif
