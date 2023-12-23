// Snipe editor. Free and open source, see licence.txt.
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// A display object represents the main window, with layout measurements. The
// display is a viewport of size rows x cols onto a notional grid of characters
// representing the entire document. Pad is the number of pixels round the edge
// of the window. Scroll is the pixel scroll position, from the overall top of
// the text, and scrollTarget is the pixel position to aim for when smooth
// scrolling. Magnify is 1 except on Mac retina screens, where it is 2 (i.e.
// 2x2 real pixels for each virtual screen pixel. Event handling is delegated
// to a handler object (and thread).

struct display {
    ALLEGRO_DISPLAY *d;
};

// Give an error message and stop.
static void crash(char const *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

Display *newDisplay() {
    if(! al_init()) crash("Failed to initialize Allegro.");
    al_init_font_addon();
    al_init_ttf_addon();
    ALLEGRO_DISPLAY *d = al_create_display(890,530);
    if (! d) crash("Failed to create display.");
    ALLEGRO_FONT *font = al_load_ttf_font("../fonts/NotoSansMono-Regular.ttf",18,0);
    if (! font) crash("Could not load font.");
}
