#include <stdio.h>
#include <allegro5/allegro.h>

// TODO: handle settings. Pass to display.
// TODO: interpret events? Pass to display/document.


char *wkeys[] = {
    "size", "rows", "cols", "font0", "font1",
    "help0", "help1", "help2", "help3", "help4", "help5", "help6",
    NULL
};

char *kkeys[] = {
    "BACKSPACE", NULL
};

char *tkeys[] = {
    "Ground", NULL
};

int main() {
    ALLEGRO_CONFIG *cfg = al_load_config_file("../snipe.cfg");
    for (int i = 0; wkeys[i] != NULL; i++) {
        char *key = wkeys[i];
        printf("%s %s\n", key, al_get_config_value(cfg, "window", key));
    }
    for (int i = 0; kkeys[i] != NULL; i++) {
        char *key = kkeys[i];
        printf("%s %s\n", key, al_get_config_value(cfg, "keys", key));
    }
    for (int i = 0; tkeys[i] != NULL; i++) {
        char *key = tkeys[i];
        printf("%s %s\n", key, al_get_config_value(cfg, "theme0", key));
    }
}
