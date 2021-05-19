// Snipe settings. Free and open source. See licence.txt.
#include "settings.h"
#include "unicode.h"
#include <allegro5/allegro.h>
#include <stdio.h>

// Use Allegro's config file support, hold memory for items.
struct settings {
    ALLEGRO_CONFIG *cfg;
    int size, rows, cols;
    float blink;
    char **help;
    char **fontNames;
    char **fonts;
    char **colourNames;
    int *colours;
    char **styleNames;
    int *styles;
};

// Get a single config value.
static char const *get(ALLEGRO_CONFIG *cfg, char *x, char *y) {
    return al_get_config_value(cfg, x, y);
}

// Find the number of tokens in s (i.e. number of spaces plus one)
static int countTokens(char const *s) {
    int n = 1;
    for (int i = 0; i < strlen(s); i++) if (s[i] == ' ') n++;
    return n;
}

// Allocate and make a NULL terminated list of strings from a single value.
static char **tokens(ALLEGRO_CONFIG *cfg, char *x, char *y) {
    char const *s = al_get_config_value(cfg, x, y);
    int n = countTokens(s);
    char **ps = malloc((n+1) * sizeof(char *) + strlen(s) + 1);
    char *t = (char *)(ps + n + 1);
    strcpy(t, s);
    for (int i = 0; i < strlen(s); i++) if (t[i] == ' ') t[i] = '\0';
    for (int i = 0; i < n; i++) {
        ps[i] = t;
        t = t + strlen(t) + 1;
    }
    ps[n] = NULL;
    return ps;
}

// Info gathered from a section: number of keys, memory needed for key strings,
// number of tokens in the values, memory needed for tokens.
struct info { int nkeys, memkeys, ntokens, memtokens; };
typedef struct info info;

// Find the info about a section.
static info measureSection(ALLEGRO_CONFIG *cfg, char const *section) {
    ALLEGRO_CONFIG_ENTRY *iterator;
    char const *key = al_get_first_config_entry(cfg, section, &iterator);
    info x = { 0, 0, 0, 0 };
    for ( ; key != NULL; x.nkeys++) {
        x.memkeys += strlen(key) + 1;
        char const *value = al_get_config_value(cfg, section, key);
        x.ntokens += countTokens(value);
        x.memtokens += strlen(value) + 1;
        key = al_get_next_config_entry(&iterator);
    }
    return x;
}

// Allocate and make a NULL terminated list of the keys in a section.
static char **keys(ALLEGRO_CONFIG *cfg, char const *section) {
    info x = measureSection(cfg, section);
    char **ks = malloc((x.nkeys+1) * sizeof(char *) + x.memkeys);
    char *vs = (char *)(ks + x.nkeys + 1);
    ALLEGRO_CONFIG_ENTRY *iterator;
    char const *key = al_get_first_config_entry(cfg, section, &iterator);
    for (int i = 0; i < x.nkeys; i++) {
        strcpy(vs, key);
        ks[i] = vs;
        vs = vs + strlen(key) + 1;
        key = al_get_next_config_entry(&iterator);
    }
    ks[x.nkeys] = NULL;
    return ks;
}

// Get the colours, given the colour names.
static int *colours(ALLEGRO_CONFIG *cfg, char **colourNames) {
    int n = 0;
    for (int i = 0; colourNames[i] != NULL; i++) n++;
    int *cs = malloc((n + 1) * sizeof(int));
    for (int i = 0; colourNames[i] != NULL; i++) {
        char *name = colourNames[i];
        char const *v = al_get_config_value(cfg, "colours", name);
        cs[i] = strtoul(v, NULL, 16);
    }
    cs[n] = -1;
    return cs;
}

// Allocate and make a NULL terminated list of the value strings from a section,
// given the keys.
static char **values(ALLEGRO_CONFIG *cfg, char *section, char **keys) {
    int n = 0, m = 0;
    for (int i = 0; keys[i] != NULL; i++) {
        n++;
        char *key = keys[i];
        char const *v = al_get_config_value(cfg, section, key);
        m = m + strlen(v) + 1;
    }
    char **vs = malloc((n + 1) * sizeof(char *) + m);
    char *t = (char *)(vs + n + 1);
    for (int i = 0; keys[i] != NULL; i++) {
        char *key = keys[i];
        char const *v = al_get_config_value(cfg, section, key);
        strcpy(t, v);
        vs[i] = t;
        t = t + strlen(v) + 1;
    }
    vs[n] = NULL;
    return vs;
}

// Allocate and make a -1 terminated list of styles, given the style names.
static int *styles(ALLEGRO_CONFIG *cfg, char **keys) {
    int n = 0;
    for (int i = 0; keys[i] != NULL; i++) n++;
    char const *cursor = al_get_config_value(cfg, "themes", "cursor");
    int t = countTokens(cursor);
    for (int i = 0; keys[i] != NULL; i++) {
        char const *value = al_get_config_value(cfg, "themes", keys[i]);
    }
}

settings *newSettings(files *fs) {
    settings *ss = calloc(1, sizeof(settings));
    char *filename = join(2, prefsDir(fs), "snipe.cfg");
    ss->cfg = al_load_config_file(filename);
    free(filename);
    if (ss->cfg == NULL) {
        filename = join(2, installDir(fs), "snipe.cfg");
        ss->cfg = al_load_config_file(filename);
        free(filename);
    }
    if (ss->cfg == NULL) crash("can't load %s", filename);
    ss->size = atoi(get(ss->cfg, "", "size"));
    ss->rows = atoi(get(ss->cfg, "", "rows"));
    ss->cols = atoi(get(ss->cfg, "", "cols"));
    ss->blink = atof(get(ss->cfg, "", "blink"));
    ss->help = tokens(ss->cfg, "", "help");
    ss->fontNames = keys(ss->cfg, "fonts");
    ss->fonts = values(ss->cfg, "fonts", ss->fontNames);
    ss->colourNames = keys(ss->cfg, "colours");
    ss->colours = colours(ss->cfg, ss->colourNames);
    ss->styleNames = keys(ss->cfg, "themes");
    return ss;
}

void freeSettings(settings *ss) {
    if (ss->cfg != NULL) al_destroy_config(ss->cfg);
    if (ss->help != NULL) free(ss->help);
    if (ss->fontNames != NULL) free(ss->fontNames);
    if (ss->fonts != NULL) free(ss->fonts);
    if (ss->colourNames != NULL) free(ss->colourNames);
    if (ss->colours != NULL) free(ss->colours);
    if (ss->styleNames != NULL) free(ss->styleNames);
    free(ss);
}

int size0(settings *ss) { return ss->size; }
int rows0(settings *ss) { return ss->rows; }
int cols0(settings *ss) { return ss->cols; }
float blink0(settings *ss) { return ss->blink; }
char **help0(settings *ss) { return ss->help; }
char **fonts0(settings *ss) { return ss->fonts; }
int *colours0(settings *ss) { return ss->colours; }

#ifdef settingsTest

int main(int n, char const *args[]) {
    files *fs = newFiles(args[0]);
    settings *ss = newSettings(fs);
    assert(rows0(ss) == 24);
    assert(cols0(ss) == 80);
    assert(blink0(ss) == 0.5);
    for (int i = 0; ss->styleNames[i] != NULL; i++) printf("%s\n", ss->styleNames[i]);
    freeSettings(ss);
    freeFiles(fs);
    return 0;
}

#endif
