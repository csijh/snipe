// Snipe editor. Free and open source, see licence.txt.

struct brackets;
typedef struct brackets Brackets;

// Create a new brackets object, initially empty.
Brackets *newBrackets(byte *types);

// Delete brackets in the n bytes before the gap.
void deleteBrackets(Brackets *ts, int n);

// Insert n bracket bytes at the start of the gap, and prepare for scanning.
void insertBrackets(Brackets *ts, int n);

// Check if the top of the unmatched opener stack matches a closer type.
bool matchTop(Brackets *ts, int type);

// Give bracket at t the given type, and do any appropriate bracket matching.
void setBracket(Brackets *ts, int t, int type);

// TODO: Move cursor.
// TODO: Read for display.
