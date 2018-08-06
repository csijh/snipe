// The Snipe editor is free and open source, see licence.txt.

// A theme provides colours for syntax highlighting. The available themes are
// defined in the settings.
struct theme;
typedef struct theme theme;

// A colour is in RGBA format, accessed by pointer to avoid byte order issues.
struct colour;
typedef struct colour colour;

// Declare the style type, avoiding inclusion of the style header.
typedef unsigned char style;

// Load the default theme from the installation directory.
theme *newTheme(void);

// Free up a theme object and its data.
void freeTheme(theme *t);

// Cycle through the available themes.
void nextTheme(theme *t);

// Find the colour for the given style.
colour *findColour(theme *t, char style);

// Find the components of a colour.
unsigned char red(colour *c);
unsigned char green(colour *c);
unsigned char blue(colour *c);
unsigned char opacity(colour *c);
