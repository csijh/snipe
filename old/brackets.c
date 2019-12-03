// The Snipe editor is free and open source, see licence.txt.

// For each bracket, store the character and the position in the text.
struct bracket { int at; char ch; };
typedef struct bracket;

// Bracket matching is done forwards from the start of the text to the cursor,
// and backwards from the end of the text to the cursor. The unmatched open
// brackets before the cursor, and the unmatched close brackets after the
// cursor, are stored. The number of brackets stored is determined roughly by
// the indent level at the cursor position, so is not likely to be large. A gap
// buffer is used, with the gap at the cursor position. That makes it possible
// to make cheap incremental changes. The total number of bytes in the text is
// tracked in max, and entries after the gap are stored relative to max. The
// current cursor position is tracked in the at field.
struct brackets {
    bracket *data;
    int lo, hi, top;
    int max, at;
};

brackets *newBrackets() {
    brackets *bs = malloc(sizeof(brackets));
    bracket *data = malloc(6 * sizeof(bracket));
    *bs = (brackets) { .lo=0, .hi=6, .top=6, .max=0, .at=0, .data=data };
    return bs;
}

void freeBrackets(brackets *bs) {
    free(bs->data);
    free(bs);
}

// Scan text from start to cursor.
static void matchForward(brackets *bs, int start, int n, char s[n]) {
    for (int i = 0; i < ?; i++) {
        int at = ...
        bracket *last = ...
        char ch = s[i];
        // open v close, last == NULL, last->ch, priority.
        // mark mismatched and then ignore.
    }
}

void changeBrackets(brackets *bs, op *o) {
    // range TODO fix, and check same as style.
    int start = atOp(o);
    int end = start + lengthOp(o);
    int cursor = atOp(o);
    // Update max. (Does it affect start, end?)
    // Delete entries between start and end.
    // Ensure range covers cursor.
    // Match from start to cursor, adding to gap start.
    // Match back from end to cursor, adding to gap end.
    // Apply style changes to all the brackets stored.
    // Make sure previously marked brackets, not stored, remain marked.
}

// TODO: in op, store the cursor position as part of the edit.
