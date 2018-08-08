// The Snipe editor is free and open source, see licence.txt.

// Read the settings from settings.txt and make the results available to all the
// Snipe modules. Each line in settings.txt consists of one of the following
// constants then a space then a value. A value which is a file path is relative
// to the installation directory and uses / as a separator. There may be several
// Theme entries, which the user can cycle through with CTRL+Enter.
enum setting {
    Font,        // Path of font file
    FontSize,    // Initial size of font in pixels
    Rows,        // Number of rows displayed in the window
    Columns,     // Number of columns displayed in the window
    BlinkRate,   // Number of seconds between cursor blinks (or 0)
    MinScroll,   // Min scroll speed in pixels per frame, e.g. 1
    MaxScroll,   // Max scroll speed in pixels per frame, e.g. 10
    Map,         // Path to file containing key/mouse mappings
    HelpCommand, // Command to start up help page in browser, taken from
                 // HelpLinux, HelpMacOS, HelpWindows in map file
    Testing,     // Whether to print events and actions (on or off)
    Theme        // Theme file(s)
};
typedef int setting;

// Get the value of a setting (other than Theme) as a string.
char *getSetting(setting s);

// Get the i'th theme filename, or NULL if there are no more.
char *getThemeFile(int i);
