// Cursors and selections. Free and open source. See licence.txt.
#include "cursors.h"

// Multiple cursors are held in a flexible array of structures. The current
// cursor is tracked to supporting dragging.
struct cursors {
//    text *t;
    int length, max, current;
    cursor *a;
};

// Adjust the end points of the cursors as the result of an insertion. If an
// endpoint is on the insertion point, put it after the insertion if the cursor
// has a selection to the right or no selection.
static void postInsert(cursors *cs, int at, int n) {
    for (int i = 0; i < cs->length; i++) {
        cursor *c = &cs->a[i];
        if (c->from < at && c->to > at) c->to = c->to + n;
        else if (c->from >= at) {
            c->from = c->from + n;
            c->to = c->to + n;
        }
    }
}

// ----------
// Types of insertion:
// a) at a cursor (e.g. type/paste)
// b) to 'replace' a selection (collapse right)
// c) final newline (has no effect on cursors)
// d) change of indent (can only be cursor at end of indent)
// e) add semicolon (should be attached to newline and indent as one 'char')


// Each cursor has a selector. When the two are at the same position, there is
// no selection and they move together. Cursors are stored in order of the
// positions of their selectors. No two selectors are at the same position, so
// the selector position is used to specify edit operations. A cursor can
// temporarily be anywhere relative to its selector. But after a drop following
// a drag, or after a multi-cursor edit, overlapping cursors are collapsed. A
// cursor can only be at the same position as another cursor or its selector if
// there is no visual ambiguity, i.e. in the three situations ...|... or
// ...|...| or |...|...

// To make insertions and deletions independent of cursors and to enable
// undo/redo, there is a convention about what happens to cursors during
// insertions and deletions. A selector at the insertion position, or a cursor
// at the insertion position where its selector is at or to the right of the
// insertion position, moves to the end of the inserted text. But a cursor at
// the insertion position whose selector is to the left doesn't move. With a
// deletion, it is clear where all the positions end up. But if that causes
// overlap, explicit changes are made to the cursors first.

// POST-INSERT REPAIR: After the insert, if the inserted text starts with \n,
// and there are spaces to the left of the insertion position which are not
// needed to hold a cursor, delete them (there can only be a cursor/selector at
// the left end of the deletion range, so the deletion can't cause any
// deselection or overlap). After the insert, EOT can be patched up. The
// insertion of a final newline, or deletion of unneeded excess final newlines
// can't change any cursors.

// A multi-cursor edit is done to each cursor in turn, then the cursors are
// collapsed as necessary. To cooperate with undo/redo, the convention on
// insertion is that any cursor or selector at the insertion point moves to the
// right of the inserted text. Insertion where two cursors may be at the same
// position is implemented carefully to preserve this convention. Before a
// deletion, any cursors or selectors within range are moved explicitly to the
// right hand end of the deletion as necessary, so that undo recovers them.

// Multi-cursor Insert RTL: collapseR; insert; delete old selection
// ...|.I.| -> ...|???| -> ...|???x| -> ...|x| <collapse R>
// |...|.I. -> |...???| -> |...???x| -> |...x|
// ...|.I. -> ...|???| -> ...  <collapse R>

// Delete left (LTR):
// If a cursor/selector is inside:
//     selector: report a move left to the history, move it left, maybe delete
//     cursor: delete
//     do the delete

// Delete right (what about cursor with trailers?)

// Add cursor(at)         [cursor and selector]
// Del cursor(at)         [at is selector posn]
// Select(at,to)          [at is selector position, to is cursor position]
//                          [at == to is cancel selection]
// Move cursor(at,to)     [cursor and selector]
// Move Selector(at,to)   [e.g. to collapse in opposite direction]

// Prepare to do a deletion forwards from 'at' to 'to'. To ensure that it can be
// undone by an insert, deal in advance with any cursors which overlap the
// deletion, assuming no cursors overlap before the call.
static void preDelete(cursors *cs, int at, int to, edits *e) {



    int i = 0, j = cs->nCursors;
    while (i < j && cs->a[i].from < at && cs->a[i].at < at) i++;
    while (j > i && cs->a[j].from >= to && cs->a[j].at >= to) j--;
    while (j - i > 2) { deleteCursor(i + 1); j--; }
    if (j - i == 0) return;
    else if (j - i == 1 && cs->a[i].from < at) {
        int p = cs->a[i].at;
        if (p < to) SETAT(i, to);
    }
    else if (j - i == 1 && cs->a[i].from == at) {
        int p = cs->a[i].at;
        if (p < at) SETFROM(i, to);
        else if (p < to) SETFROMAT(i, to);
        else SETFROM(i, to);
    }



    for (int i = 0; i < cs->nCursors; i++) {
        cursor *c = &cs->a[i];
        if (c->from < at && c->at < at) continue;
        if ()

        cursor *d = &cs->a[i+1];



        if (c->from > to && c->at > to) continue;
        if (c->from >= at && )
                deleteCursor(i);
                i--;
            }
            else right = i;
        }
        else if (c->at <= at+n) right = i;
        else {
            c->from = c->from - n;
            c->at = c->at - n;
        }
    }
    // TODO pass the deletion to text.
}
