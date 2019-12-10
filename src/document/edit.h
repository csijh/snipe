// Primitive edit operations. Free and open source. See licence.txt.

// Each user action becomes a sequence of edits to a document. The edits include
// automatic adjustments such as re-indenting. The state of a document consists
// of its text, and a set of cursors with possible selectors. The edits and
// their rules are designed so that the edits are inverses of each other for
// undo, and that an undo restores cursors and selectors as well as text.

// DoInsert(at,s) is an insertion of a string at a given position. If there is a
// cursor at that position, it moves to the end of the insertion. DoDelete(at,s)
// is a deletion of a string at the given position. If there is a cursor in the
// range deleted, it must be at the end of the string. The string s must be
// filled in from the text when the edit is created.

// DoAdd(at) adds a new cursor at the given position. DoCancel(at) removes a
// cursor. DoSelect(at,to) moves the cursor and selects the text covered.
// DoDeselect(at,to) moves the cursor to its selector, cancelling it.
// DoMove(at,to) moves a cursor (self-inverse). DoEnd() marks the end of the
// edits for one user action.

enum editOp {
    DoInsert, DoDelete, DoAdd, DoCancel, DoSelect, DoDeselect, DoMove, DoEnd
};

struct edit;
typedef struct edit edit;

// Create or free an edit structure.
edit *newEdit();
void freeEdit(edit *e);

// Get an edit's operation, 'at' position, 'to' position, or length of text.
int opEdit(edit *e);
int atEdit(edit *e);
int toEdit(edit *e);
int lengthEdit(edit *e);

// Set up an edit. For insertion, the string s is cleaned in place to remove
// invalid UTF-8 bytes, nulls, returns, or trailing spaces.
void insertEdit(edit *e, int at, int n, char s[n]);
void deleteEdit(edit *e, int at, int n, char s[n]);
void addEdit(edit *e, int at);
void cancelEdit(edit *e, int at);
void selectEdit(edit *e, int at, int to);
void deselectEdit(edit *e, int at, int to);
void moveEdit(edit *e, int at, int to);
void endEdit(edit *e);

// Get the string from an insertion or deletion, valid only during the edit.
char const *stringEdit(edit *e);

// Copy the string from an insertion or deletion to s.
void copyEdit(edit *e, char *s);
