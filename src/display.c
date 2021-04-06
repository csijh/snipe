// Snipe graphics display. Free and open source. See licence.txt.
#include "display.h"
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

struct display {
    ALLEGRO_DISPLAY *display;
    ALLEGRO_FONT *font;
    ALLEGRO_COLOR bg, fg[10];
};

// Crash the program if there is any failure.
static void fail(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

// Check result of a function which must succeed.
static void try(bool b, char *s) { if (!b) fail(s); }

// Create a new display.
display *newDisplay() {
    display *d = malloc(sizeof(display));
    try(al_init(), "Failed to initialize Allegro.");
    al_init_font_addon();
    al_init_ttf_addon();
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
    d->display = al_create_display(890, 530);
    try(d->display != NULL, "Failed to create display.");
    d->font = al_load_ttf_font("DejaVuSansMono.ttf", 18, 0);
    try(d->font != NULL, "Failed to load 'DejaVuSansMono.ttf'.");
    d->bg = al_map_rgb(0, 0x2b, 0x36);
    d->fg[0] = al_map_rgb(0x83, 0x94, 0x96);
    d->fg[1] = al_map_rgb(0x83, 0x94, 0x96);
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

void drawPage(display *d, char *bytes, unsigned char *tags) {
    ALLEGRO_USTR_INFO ui;
    al_clear_to_color(d->bg);
    int lh = al_get_font_line_height(d->font);
    float x = 4, y = 0;
    for (int i = 0; bytes[i] != '\0'; i++) {
        char ch = bytes[i];
        if (ch == '\n') {
            y = y + lh;
            x = 4;
            continue;
        }
        ALLEGRO_COLOR col = d->fg[tags[i]];
        al_ref_buffer(&ui, &bytes[i], 1);
        al_draw_ustr(d->font, col, x, y, 0, &ui);
        x += al_get_ustr_width(d->font, &ui);
    }
    al_flip_display();
}

#ifdef displayTest

int main() {
    display *d = newDisplay();
    unsigned char styles[] = { 0, 1, 1 };
    drawPage(d, "abc", styles);
    al_rest(5);
    freeDisplay(d);
    al_uninstall_system();
    return 0;
}

#endif
