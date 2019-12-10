// Snipe bracket matcher. Free and open source. See licence.txt.

// A brackets object keeps track of the brackets in the text, and does
// incremental bracket matching.
brackets *newBrackets();
void freeBrackets(brackets *bs);

// Track insertions, deletions and cursor movements.
void changeBrackets(brackets *bs, op *o);

// ----------------------------------------------------------------------------
// Style:
// Need conversion to/from ASCII characters.
// Need two bits: comment override, string override. OR attach them to lines.
// Need a bit or extra styles: mismatched 'bracket'.
// ----------------------------------------------------------------------------
// Store unmatched brackets up to cursor. (That is indenters plus unmatched
// brackets on current line.) Don't need line boundaries! Except do need tokens,
// and tokens need lines.

// Insert character before cursor: = normal algorithm
// Non-bracket: nothing happens.
// Opener: gets pushed on stack.
// Closer: gets (mis)matched with open bracket(s) on the stack.
//      The matched/mismatched brackets are dropped out.

// Delete character before cursor.
// Non-bracket: nothing happens.
// Opener: must be on top of stack, gets popped.
// Closer:
//      must be (mis)matched with an opener not on the stack
//      and the opener must be since the top opener
//      therefore search back from cursor to position of top opener

// Move cursor right = normal algorithm
// Move cursor left = delete

// The two stacks are matched out to in. Can that also be done incrementally?
// Matched outer brackets are paired. What about a mismatch:
//    {.... {   |   ] ....}
// It looks like the ] should be mismatched.
// The ] can point to the opener that mismatched it.
// The algorithm can continue until all stacked brackets are dealt with.

//    {.... { .. [   |   } .. ] ....}        ( [ { " /*
//    1                             1
//    1     !                 x     1
//    1     2            2    x     1
//    1     2    x      !2    x     1
// Just look at curlies:
//    {{{{{{{    |    }}}}
//    1234xxx         4321
// Anything of less priority must be between inner {}, so is mismatched by
// one of them. A new curly invalidates anything lesser. A new curly on the
// left just accumulates. But a new curly on the right matches a long way up:
//    {{{{{{{    |    }}}}}
//    12345xx         54321
// So: just mark as matched or mismatched.

// Now what about incremental? Try push left opener. If top right is matched,
// the new opener is mismatched.

// Normal forward algorithm:
// See open bracket, push it.
// See close bracket, try to match with top.
// {) => drop the close bracket.
// () => match and drop both.
// (} => drop open bracket and loop.
// Either match and drop out, or mismatch and drop out.

// Normal reverse algorithm:
// See open bracket, pop it.
// See close bracket = top. Can't happen.
// See close bracket not top. MAYBE reconstruct what happened since the top.
