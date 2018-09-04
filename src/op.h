// The Snipe editor is free and open source, see licence.txt.

// An op specifies whether a change is an edit or adjustment, and how it
// relates to a cursor (so that the cursor can be reconstructed on undo).
enum op {
    INS,            // Insertion, before the cursor if at a cursor position
    DEL_L, DEL_R,   // Delete to the left or the right of the cursor
    DEL_LR, DEL_RL, // Delete a left-to-right or right-to-left selection
    INS_A, DEL_A,   // Adjustments
    COUNT_OPS
};
typedef int op;
