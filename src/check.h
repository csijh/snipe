// The Snipe editor is free and open source. See licence.txt.
#include <stdbool.h>

// Crash with a formatted error message. A newline is added.
void crash(char const *fmt, ...);

// Check e.g. the result of a function call, and crash if it failed.
void check(bool ok, char const *fmt, ...);
