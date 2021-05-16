// Snipe settings. Free and open source. See licence.txt.
#include "files.h"

struct settings;
typedef struct settings settings;

settings *newSettings(files *fs);
void freeSettings(settings *ss);
