// Snipe settings. Free and open source. See licence.txt.
#include "files.h"

struct settings;
typedef struct settings settings;

// Create a settings object from snipe.cfg in the user's preference directory,
// or from snipe.cfg in the snipe installation directory.
settings *newSettings(files *fs);

// Free the settings object.
void freeSettings(settings *ss);

// Get the settings items: font size in pixels, screen size in characters, blink
// rate in seconds, name of command to show help files in browser.
int size0(settings *ss);
int rows0(settings *ss);
int cols0(settings *ss);
float blink0(settings *ss);

// Get the NULL terminated list of browser commands to try, for displaying help.
char **help0(settings *ss);

// Get the font filenames as a NULL terminated array. A name starting with '+'
// is a fallback for the previous font.
char **fonts0(settings *ss);

// Get the colours as a -1 terminated array of rgb values.
int *colours0(settings *ss);

// Get the themes as a -1 terminated array of styles. Each style consists of
// four bytes: a tag character, a foreground colour index, a background colour
// index, and a font index. For n themes, there are n styles for each tag.
int *themes0(settings *ss);

// Get the keymap settings as a list of strings. Each pair of strings is an
// event name, and an action name.
char **keys0(settings *ss);
