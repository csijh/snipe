// Snipe graphics display. Free and open source. See licence.txt.
#include "display.h"
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

struct display {
    ALLEGRO_DISPLAY *display;
    ALLEGRO_FONT *font;
    ALLEGRO_COLOR bg[4], fg[31];
    int width, height;
    int charWidth, lineHeight, pad;
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

// Themes. First bg shade is normal, the second is for selected text, the third
// is inverted for bad/mismatched tokens. The first fg shade is for normal text,
// the second is for comments, the third is strong text, the rest are for
// different token types.

// Solarized-light theme.
shade bgl[] = { base3, base2, base03, base3 };
shade fgl[] = {
    base00, base1, base01, yellow, orange, cyan, blue, red, magenta, violet,
    green, base00, base00, base00, base00, base00, base00, base00, base00,
    base00, base00, base00, base00, base00, base00, base00, base00, base00,
    base00, base00, base00
};

// Solarized-dark theme.
shade bgd[] = { base03, base02, base3, base03 };
shade fgd[] = {
    base0, base01, base1, yellow, orange, cyan, blue, red, magenta, violet,
    green, base0, base0, base0, base0, base0, base0, base0, base0, base0, base0,
    base0, base0, base0, base0, base0, base0, base0, base0, base0, base0
};

// Crash the program if there is any failure.
static void fail(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

// Check result of a function which must succeed.
static void try(bool b, char *s) { if (!b) fail(s); }

static void setTheme(display *d, shade *bg, shade *fg) {
    for (int i = 0; i < 4; i++) {
        shade c = bg[i];
        d->bg[i] = al_map_rgb((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF);
    }
    for (int i = 0; i < 31; i++) {
        shade c = fg[i];
        d->fg[i] = al_map_rgb((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF);
    }
}

// Create a new display.
display *newDisplay() {
    display *d = malloc(sizeof(display));
    try(al_init(), "Failed to initialize Allegro.");
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    d->font = al_load_ttf_font("DejaVuSansMono.ttf", 18, 0);
    try(d->font != NULL, "Failed to load 'DejaVuSansMono.ttf'.");
    d->charWidth = al_get_text_width(d->font, "m");
    d->lineHeight = al_get_font_line_height(d->font);
    d->pad = 4;
    d->width = d->pad + 80 * d->charWidth + d->pad;
    d->height = 24 * d->lineHeight;
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
    d->display = al_create_display(d->width, d->height);
    try(d->display != NULL, "Failed to create display.");
    setTheme(d, bgl, fgl);
    return d;
}

void freeDisplay(display *d) {
    al_destroy_font(d->font);
    al_destroy_display(d->display);
    free(d);
}

void *getHandle(display *d) {
    return d->display;
}

void clear(display *d) {
    al_clear_to_color(d->bg[0]);
}

void frame(display *d) {
    al_flip_display();
}

void drawLine(display *d, int row, char *bytes, unsigned char *tags) {
    ALLEGRO_USTR_INFO ui;
    float x = d->pad, y = row * d->lineHeight;
    int oldBg, oldFg;
    for (int i = 0; bytes[i] != '\0'; i++) {
        char ch = bytes[i];
        if ((ch & 0xC0) == 0x80) continue;
        int n = 1;
        while ((bytes[i+n] & 0xC0) == 0x80) n++;
        al_ref_buffer(&ui, &bytes[i], n);
        int bgi = (tags[i] >> 5) & 0x03;
        int fgi = tags[i] & 0x1F;
        if (fgi == 31) {
            fgi = oldFg; bgi = oldBg; x = x - d->charWidth;
        }
        ALLEGRO_COLOR fg = d->fg[fgi], bg = d->bg[bgi];
        if (bgi != 0) {
            al_draw_filled_rectangle(x,y,x+d->charWidth,y+d->lineHeight,bg);
        }
        oldFg = fgi;
        oldBg = bgi;
        if (ch != '\n') al_draw_ustr(d->font, fg, x, y, 0, &ui);
        x += d->charWidth;
    }
}

void drawPage(display *d, char *bytes, unsigned char *tags) {
    ALLEGRO_USTR_INFO ui;
    al_clear_to_color(d->bg[0]);
    float x = d->pad, y = 0;
    for (int i = 0; bytes[i] != '\0'; i++) {
        char ch = bytes[i];
        if ((ch & 0xC0) == 0x80) continue;
        if (ch == '\n') {
            y = y + d->lineHeight;
            x = d->pad;
            continue;
        }
        int n = 1;
        while ((bytes[i+n] & 0xC0) == 0x80) n++;
        al_ref_buffer(&ui, &bytes[i], n);
        int bgi = (tags[i] >> 5) & 0x03;
        int fgi = tags[i] & 0x1F;
        ALLEGRO_COLOR fg = d->fg[fgi], bg = d->bg[bgi];
        if (bgi != 0) {
            al_draw_filled_rectangle(x,y,x+d->charWidth,y+d->lineHeight,bg);
        }
        al_draw_ustr(d->font, fg, x, y, 0, &ui);
        x += d->charWidth;
    }
    al_flip_display();
}

#ifdef displayTest

// styles for bad tokens, selected text, continuation byte or combiner
enum { b = 0x47, h = 0x20, c = 0xFF };

// Different fg and bg colours.
static char *line1 = "id(12,CON) = 'xyz' 12. high // note\n";
static unsigned char styles1[] =
    { 0,0,0,5,5,0,4,4,4,0,0,0,0,6,6,6,6,6,0,b,b,b,0,h,h,h,h,0,1,1,1,1,1,1,1,0 };

// Eight 2-byte and four 3-byte characters
static char *line2 = "æ í ð ö þ ƶ ə β ᴈ ῷ ⁑ €\n";
static unsigned char styles2[] = {
    0,c,0,0,c,0,0,c,0,0,c,0,0,c,0,0,c,0,0,c,0,0,c,0,0,c,c,0,0,c,c,0,0,c,c,0,0,c,c,0 };

// A two-byte e character, and an e followed by a two-byte combiner
static char *line3 = "Raphaël Raphaël\n";
static unsigned char styles3[] = {
    0,0,0,0,0,0,c,0,0,0,0,0,0,0,0,c,c,0,0 };


int main() {
    display *d = newDisplay();
    clear(d);
    drawLine(d,0,line1,styles1);
    drawLine(d,1,line2,styles2);
    drawLine(d,2,line3,styles3);
    frame(d);
//    drawPage(d, text, styles);
    al_rest(3);
    setTheme(d, bgd, fgd);
    clear(d);
    drawLine(d,0,line1,styles1);
    drawLine(d,1,line2,styles2);
    drawLine(d,2,line3,styles3);
    frame(d);
//    drawPage(d, text, styles);
    al_rest(3);
    freeDisplay(d);
    al_uninstall_system();
    return 0;
}

#endif
