// The Snipe editor is free and open source. See licence.txt.
#include "display.h"
#include "handler.h"
#include "array.h"
#include "check.h"
#include "unicode.h"
#include "text.h"
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// TODO: scroll a long line left/right according to cursor position.

// A cell records, for a grapheme cluster drawn at a particular (row,column)
// position, the byte position in the line, and the horizontal pixel position
// across the screen. This supports conversion from (x,y) pixel coordinates
// e.g. from a mouse click to (row, column) and then (line, byte-index) text
// positions. It also supports e.g. moving the cursor left or right by one
// grapheme cluster. (Grapheme clusters are THE units of text. The Unicode
// standard has an algorithm for them, but that isn't used here because, as the
// standard says, what matters more when text is displayed are the fonts and
// the renderer. So, the currently used chain of fonts and allegro are used to
// determine grapheme clusters. The only indication given by allegro seems to
// be a zero advance for combining characters.)
struct cell { int bytes, pixels; };
typedef struct cell Cell;

// A display object represents the main window. It has a separate handler for
// events. The widow is (width x height) pixels, divided into (rows x cols)
// cells of size (rowHeight x colWidth) and with pad pixels round the edge. The
// first line of the file shown in the window is topLine, the number of pixels
// of that line which don't appear because of scrolling is scrollPixels
// (< rowHeight), and the target during smooth scrolling is scrollTarget.
struct display {
    handler *h;
    ALLEGRO_DISPLAY *window;
    ALLEGRO_FONT *font;
    ALLEGRO_COLOR theme[Caret+1];
    int width, height;
    int rows, cols;
    int rowHeight, colWidth, pad;
    int topLine, scrollPixels, scrollTarget;
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
        if (k < 0 || k > Caret) { printf("Bad theme key %s\n", key); continue; }
        if (value[0] == '#') {
            int col = (int) strtol(&value[1], NULL, 16);
            d->theme[k] = al_map_rgb((col>>16)&0xFF, (col>>8)&0xFF, col&0xFF);
        }
        else {
            Kind prev = findKind(value);
            if (k < 0 || k > Caret)  {
                printf("Bad theme value %s\n", value);
                continue;
            }
            d->theme[k] = d->theme[prev];
        }
    }
    for (int k = 0; k < Caret; k++) {
        if (k == More) continue;
        if (d->theme[k].a != 0) continue;
        char *name = kindName(k);
        if (strlen(name) == 1) continue;
        printf("No theme entry for %s\n", name);
    }
    al_destroy_config(cfg);
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
    check(al_init(), "Failed to initialize Allegro.");
    al_init_font_addon();
    al_init_ttf_addon();
    char *fontFile1 = "../fonts/DejaVuSansMono.ttf";
    char *fontFile2 = "../fonts/NotoSansSymbols2-Regular.ttf";
    d->font = al_load_ttf_font(fontFile1, 18, 0);
    check(d->font != NULL, "failed to load '%s'", fontFile1);
    ALLEGRO_FONT *font2 = al_load_ttf_font(fontFile2, 18, 0);
    check(font2 != NULL, "Failed to load '%s'.", fontFile2);
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
    d->window = al_create_display(d->width, d->height);
    check(d->window != NULL, "Failed to create display.");
    d->h = newHandler(d->window);
    return d;
}

void freeDisplay(Display *d) {
    freeHandler(d->h);
    free(d->grid);
    al_destroy_font(d->font);
    al_destroy_display(d->window);
    free(d);
    al_uninstall_system();
}

void clear(Display *d) {
    al_clear_to_color(d->theme[Ground]);
}

void frame(Display *d) {
    al_flip_display();
}

// Draw a filled rectangle (without using the primitives addon) by using a
// temporary small clipping window.
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
    drawRectangle(d->theme[bg], x, y, (d->width - x), d->rowHeight);
}

// Work out the pixel position of each byte in a line. A byte which continues a
// grapheme cluster is at the same position as its predecessor.
static void measure(Display *d, char *bytes, int *pixels) {
    int oldPos = d->pad;
    int oldCode = ALLEGRO_NO_KERNING;
    for (int i = 0; i < length(pixels); ) {
        int code = bytes[i];
        int len = 1;
        if ((code & 0x80) != 0) {
            Character cp = getUTF8(&bytes[i]);
            code = cp.code;
            len = cp.length;
        }
        pixels[i] = oldPos + al_get_glyph_advance(d->font, oldCode, code);
        printf("%d %d %c\n", i, pixels[i], bytes[i]);
        for (int j = i + 1; j < i + len; j++) pixels[j] = pixels[i];
        oldCode = code;
        oldPos = pixels[i];
        i = i + len;
    }
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
    d->grid[row][col] = (Cell) {x, 0};
    for (i = 0; i==0 || bytes[i-1] != '\n'; ) {
        Character cp = getUTF8(&bytes[i]);
        int advance = al_get_glyph_advance(d->font, d->oldCode, cp.code);
        x = x + advance;
        drawGlyph(d, x, y, cp.code, styles[i]);
        i = i + cp.length;
        d->oldCode = cp.code;
        if (advance > 0) d->grid[row][++col] = (Cell) { x, i };
    }
}

/*
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
    case 'I': return Identifier; case 'B': return Bracket; case 'V': return Value;
    case 'M': return Mark; case 'G': return Gap; case 'Q': return Quote;
    case 'C': return Comment;
    case 'v': return Value + ((Warn - Ground) << 5);
    case 'i': return Identifier + ((Select - Ground) << 5);
    default: return Identifier;
    }
}

// Abbreviated styles for testing
enum {I=Identifier, B=Bracket, V=Value, M=Mark, G=Gap, Q=Quote, C=Comment};
enum { v = Value + ((Warn - Ground) << 5) };
enum { i = Identifier + ((Select - Ground) << 5) };

// Different fg and bg colours.
static char *line1 =   "id(12,COM) = 'xyz' 12. high // note\n";
static char *styles1 = "IIBVVMVVVMGMGQQQQQGvvvGiiiiGCCCCCCCG";

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
    int *pixels = newArray(sizeof(int));
    pixels = adjust(pixels, strlen(line3));
    measure(d, line3, pixels);
    for (int i = 0; i < length(pixels); i++) printf("%d %d\n", i, pixels[i]);

    int n = al_get_glyph_advance(d->font, 'e', 776);
    printf("ad %d\n", n);
    n = al_get_glyph_advance(d->font, 776, 'l');
    printf("ad %d\n", n);

    drawLine(d, 0, line1, styles1);
    drawLine(d, 1, line2, styles2);
    drawLine(d, 2, line3, styles3);
    frame(d);
    al_rest(10);
    freeArray(pixels);
    freeDisplay(d);
    return 0;
}

#endif
