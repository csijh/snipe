// Snipe editor. Free and open source, see licence.txt.

typedef struct display Display;

// TODO: prefs.
Display *newDisplay();

void freeDisplay(Display *d);

void setTitle(Display *d, char const *title);

int pageRows(Display *d);
int pageCols(Display *d);
int firstRow(Display *d);
int lastRow(Display *d);

// TODO: x, y, s
int getEvent(Display *d);

// TODO: internalize? rowCol type?
int charPosition(Display *d, int x, int y);

// Create an image of a line from its row number, text, and style info.
void drawLine(display *d, int row, int n, char *line, char *styles);

// Make recent changes appear on screen, with a vertical sync delay.
void showFrame(display *d);

// Carry out the given action, if relevant.
void actOnDisplay(display *d, action a, int x, int y, char const *s);
