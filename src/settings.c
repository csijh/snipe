// Snipe settings. Free and open source. See licence.txt.
#include "settings.h"
#include <allegro5/allegro.h>
#include <stdio.h>

struct settings { ALLEGRO_CONFIG *cfg; };

settings *newSettings(files *fs) {
    settings *ss = malloc(sizeof(settings));
    char *filename = join(2, installDir(fs), "settings.cfg");
    ss->cfg = al_load_config_file(filename);
    printf("ns %p\n", (void *) ss->cfg);
    char const *size = al_get_config_value(ss->cfg, "", "size");
    printf("ns size %s\n", size);
    free(filename);
    return ss;
}

void freeSettings(settings *ss) {
    al_destroy_config(ss->cfg);
    free(ss);
}

#ifdef settingsTest

int main(int n, char const *args[]) {
    files *fs = newFiles(args[0]);
    settings *ss = newSettings(fs);
    freeSettings(ss);
    freeFiles(fs);
    return 0;
}

#endif
