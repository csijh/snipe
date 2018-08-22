// The Snipe editor is free and open source, see licence.txt.

// Store an array of information about lines. For each line, the position in
// the text, the state that scanning should be started in, and the running
// indent amount are stored. In addition, the array keeps track of which lines
// have valid information and which need to be re-processed.

struct lines;
typedef struct lines lines;

// Create or free an array of lines.
lines *newLines();
void freeLines(lines *ls);

// Insert n new lines starting at row r. Pass a pointer to the lines variable,
// so that it can be updated if the array is moved. Return the array as well
// for convenience. Invalidate rows from r onwards.
lines *addLines(lines **pls, int r, int n);

// Delete n lines starting at row r. Invalidate rows from r onwards.
void cutLines(lines *ls, int r, int n);

// Find the number of lines, find the number of valid lines, or invalidate all
// lines from row r onwards.
int countLines(lines *ls);
int validLines(lines *ls);
void invalidateLines(lines *ls, int r);

// Get the info for a line, where 0 <= row <= countLines.
int lineStart(lines *ls, int row);
int lineEnd(lines *ls, int row);
int lineLength(lines *ls, int row);
int lineState(lines *ls, int row);
int lineIndent(lines *ls, int row);

// Set the info for a line.
void setLineEnd(lines *ls, int row, int p);
void setLineEndState(lines *ls, int row, int s);
void setLineEndIndent(lines *ls, int row, int i);
