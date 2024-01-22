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

// TODO: take fonts and measurements from (theme?) settings.

// A display object represents the editor's main window. It is (width x height)
// pixels, divided into (rows x cols) cells of size (rowHeight x colWidth) and
// with (pad) pixels on the left and right edges. The number of rows displayed
// may be (rows+1) because of scrolling. The number of columns is notional,
// because the fonts may be proportional, and even monospaced fonts may have
// exceptions. The first line of the file shown in the window is topRow, the
// number of vertical pixels of that line which don't appear because of
// scrolling is scrollHeight (< rowHeight), and the target during smooth
// scrolling is scrollTarget. The pixels array stores the pixel positions of
// the displayed text. There is a separate handler for events.
struct display {
    ALLEGRO_DISPLAY *window;
    ALLEGRO_COLOR theme[Caret+1];
    ALLEGRO_FONT *font;
    int width, height;
    int rows, cols;
    int rowHeight, colWidth, pad;
    int topRow, scrollPixels, scrollTarget;
    int **pixels;
    handler *h;
};

// Create or replace the theme.
static void newTheme(Display *d, char *path) {
    for (int k = 0; k < Caret; k++) d->theme[k].a = 0;
    ALLEGRO_CONFIG *cfg = al_load_config_file(path);
    ALLEGRO_CONFIG_ENTRY *entry;
    char const *key = al_get_first_config_entry(cfg, NULL, &entry);
    for ( ; key != NULL; key = al_get_next_config_entry(&entry)) {
        char const *value = al_get_config_value(cfg, NULL, key);
        int k = findKind(key);
        if (k < 0 || k > Caret) { printf("Bad theme key %s\n", key); continue; }
        if (value[0] == '#') {
            int col = (int) strtol(&value[1], NULL, 16);
            d->theme[k] = al_map_rgb((col>>16)&0xFF, (col>>8)&0xFF, col&0xFF);
        }
        else {
            int prev = findKind(value);
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

// Create a new display.
Display *newDisplay() {
    check(al_init(), "Failed to initialize Allegro.");
    al_init_font_addon();
    al_init_ttf_addon();
    Display *d = malloc(sizeof(Display));
    al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
    check(d->window != NULL, "Failed to create display.");
    newTheme(d, "../themes/solarized-dark.txt");
    d->font = NULL;
    char *fontFile1 = "../fonts/NotoSansMono-Regular.ttf";
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
    d->pixels = newArray(sizeof(int *));
    d->window = al_create_display(d->width, d->height);
    d->h = newHandler(d->window);
    return d;
}

void freeDisplay(Display *d) {
    freeHandler(d->h);
    al_destroy_font(d->font);
    al_destroy_display(d->window);
    for (int r = 0; r < length(d->pixels); r++) freeArray(d->pixels[r]);
    freeArray(d->pixels);
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

// Draw a background for the glyph at (x,y).
static void drawBackground(Display *d, int bg, int x, int y, int w) {
    drawRectangle(d->theme[bg], x, y, w, d->rowHeight);
}

// Draw a glyph given its Unicode code point, with possible preceding caret and
// change of background. Change the background back to the default after \n.
static void drawGlyph(Display *d, int x, int y, int w, int code, int style) {
    int fg = foreground(style), bg = background(style);
    bool caret = hasCaret(style);
    if (caret) drawCaret(d, x, y);
    if (bg != Ground) drawBackground(d, bg, x, y, w);
    if (code != '\n') al_draw_glyph(d->font, d->theme[fg], x, y, code);
}

// Find the pixel positions of a row of text. This allows (x,y) window
// coordinates to be converted to (line,index) text coordinates, allowing for
// proportional fonts or occasional variations within monospaced fonts. Allegro
// and its standard addons support Unicode characters, but not complex text
// (no right-to-left text, no combiners, no grapheme clusters, no emojis), so
// currently one character is one UTF-8 byte sequence, i.e. one code point. A
// continuation byte is at the same position as its predecessor.
static void measure(Display *d, int row, char *bytes) {
    while (length(d->pixels) <= row) {
        d->pixels = adjust(d->pixels, +1);
        d->pixels[length(d->pixels) - 1] = newArray(sizeof(int));
    }
    d->pixels[row] = resize(d->pixels[row], length(bytes));
    int *pixels = d->pixels[row];
    int oldPos = 0;
    int oldCode = ALLEGRO_NO_KERNING;
    for (int i = 0; i < length(bytes); ) {
        int code = bytes[i];
        int len = 1;
        if ((code & 0x80) != 0) {
            Character cp = getUTF8(&bytes[i]);
            code = cp.code;
            len = cp.length;
        }
        d->pixels[row] = resize(pixels, length(bytes));
        pixels[i] = oldPos + al_get_glyph_advance(d->font, oldCode, code);
        for (int j = i + 1; j < i + len; j++) pixels[j] = pixels[i];
        oldCode = code;
        oldPos = pixels[i];
        i = i + len;
    }
}

// Shift a row of text if necessary to make the (last) caret visible.
static void shift(Display *d, int row, byte *styles) {
    int *pixels = d->pixels[row];
    int caret = 0;
    for (int i = 0; i < length(styles); i++) {
        if (hasCaret(styles[i])) caret = pixels[i];
    }
    int shift = 0;
    for (int i = 0; i < length(pixels); i++) {
        if (caret - pixels[i] <= d->width - 2 * d->pad) {
            shift = pixels[i];
            break;
        }
    }
    for (int i = 0; i < length(pixels); i++) pixels[i] = pixels[i] - shift;
}

// Draw continuation markers in the left or right margins of a row.
static void drawMarkers(Display *d, int row) {
    int *pixels = d->pixels[row];
    int y = row * d->rowHeight;
    if (pixels[0] < 0) {
        drawRectangle(d->theme[Warn], 0, y, d->pad, d->rowHeight);
    }
    if (pixels[length(pixels) - 1] > d->width-2*d->pad) {
        drawRectangle(d->theme[Warn], d->width-d->pad, y, d->pad, d->rowHeight);
    }
}

// When drawing a row, x is the pixel position across the screen, col is the
// cell position in the grid. and i is the byte position in the text.
void drawRow(Display *d, int row, char *bytes, byte *styles) {
    measure(d, row, bytes);
    shift(d, row, styles);
    int y = row * d->rowHeight;
    for (int i = 0; i < length(bytes); ) {
        int code = bytes[i];
        int len = 1;
        if ((code & 0x80) != 0) {
            Character cp = getUTF8(&bytes[i]);
            code = cp.code;
            len = cp.length;
        }
        int x = d->pad + d->pixels[row][i];
        int w = 0;
        if (i < length(bytes)-1) w = d->pixels[row][i+1] - d->pixels[row][i];
        drawGlyph(d, x, y, w, code, styles[i]);
        i = i + len;
    }
    drawMarkers(d, row);
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
static byte convert(char ch) {
    switch (ch) {
    case 'I': return Identifier; case 'B': return Bracket; case 'V': return Value;
    case 'M': return Mark; case 'G': return Gap; case 'Q': return Quote;
    case 'C': return Comment;
    case 'v': return Value + ((Warn - Ground) << 5);
    case 'i': return Identifier + ((Select - Ground) << 5);
    case 'c': return Identifier + (1 << 7);
    default: return Identifier;
    }
}

// Different fg and bg colours.
static char *line0 =   "id(12,COM) = 'xyz' 12. high // note\n";
static char *styles0 = "IIBVVMVVVMGMGQQQQQGvvvGiiiiGCCCCCCCG";

// Eight 2-byte and four 3-byte characters
static char *line1 =   "æ í ð ö þ ƶ ə β ᴈ ῷ ⁑ €\n";
static char *styles1 = "IIGIIGIIGIIGIIGIIGIIGIIGIIIGIIIGIIIGIIIG";

// A two-byte e character, and an e with a two-byte combiner that doesn't work.
static char *line2 =   "ëë\n";
static char *styles2 = "IIIIIIG";

// A line of exactly 80 characters.
static char *line3 =
    "1234567890123456789012345678901234567890\
1234567890123456789012345678901234567890\n";
static char *styles3 =
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\
VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVG";

// A line of 81 characters - should have an overflow marker.
static char *line4 =
    "1234567890123456789012345678901234567890\
12345678901234567890123456789012345678901\n";
static char *styles4 =
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\
VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVG";

// A line of 81 characters with end caret - should have an underflow marker.
static char *line5 =
    "1234567890123456789012345678901234567890\
12345678901234567890123456789012345678901\n";
static char *styles5 =
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\
VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVc";

static void drawLine(Display *d, int row, char *line, char *s) {
    char *bytes = newArray(sizeof(char));
    bytes = resize(bytes, strlen(line));
    strncpy(bytes, line, length(bytes));
    byte *styles = newArray(sizeof(char));
    styles = resize(styles, length(bytes));
    for (int i = 0; i < length(styles); i++) styles[i] = convert(s[i]);
    drawRow(d, row, bytes, styles);
    freeArray(styles);
    freeArray(bytes);
}

int main() {
    Display *d = newDisplay();
    clear(d);
    drawLine(d, 0, line0, styles0);
    drawLine(d, 1, line1, styles1);
    drawLine(d, 2, line2, styles2);
    for (int c = 0; c < length(d->pixels[2]); c++)
        printf("%d %d %02x\n", c, d->pixels[2][c], line2[c]&0xFF);
    drawLine(d, 3, line3, styles3);
    drawLine(d, 4, line4, styles4);
    drawLine(d, 5, line5, styles5);
    frame(d);
    al_rest(10);
    freeDisplay(d);
    return 0;
}

#endif
