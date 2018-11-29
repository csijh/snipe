Bracket matching
----------------
It is well known how to check that brackets are matched. But if mismatched
brackets are allowed, as they must be in a normal editor, and if they are to
be marked, e.g. in red, then complications arise.

    - Can the number of brackets marked as mismatched be minimized?
    - Are there ambiguities?
    - Do different brackets have different priorities?
    - How does bracket matching interact with scanning?
    - How does bracket matching affect auto-indenting?
    - Can the algorithm be made incremental and efficient on each edit?

In the example (() it isn't clear which open bracket to mark as a mismatch.
From the user's point of view, it is probably the one which has been typed most
recently, but it not sensible for bracket matching to depend on the order
things are typed. It is better to treat the issue as an invariant being
maintained, especially when it is used for things like auto-indenting. Marking
the first bracket as a mismatch fits well with the usual matching algorithm,
and feels natural if the user types a new expression with unmatched brackets
((( and then types the close brackets one at a time. It feels slightly less
natural when a new open bracket is typed inside an existing matching pair
(..(...) but  this is a minor and short-lived effect.

In the example [(] the normal matching algorithm wouldn't match any
brackets. But matching the square brackets would reduce the number of
mismatches, and feel more natural in practical situations where the inner
bracket has just been typed. With [((])) it should be the round brackets that
match if the mismatches are to be minimised. An examples such as
[[[[((((]]]] shows that brackets with an arbitrary number of unmatched brackets
in between might need to be matched. That doesn't feel as if it would lead to a
simple algorithm, or a simple explanation to users. In addition, with [(]),
there is an ambiguity because one pair of brackets needs to be matched, but it
isn't clear which.

Perhaps the best approach to cope with these problems is to have a hierarchy of
bracket types, e.g. () being the most local and {} the most global. A sequence
{(((} might occur naturally when a new line is being typed within a block, so
it makes sense to match {} and mark ((( as mismatched. whereas ({{{) is much
less likely and not unreasonable to mark () as mismatched. This doesn't prevent
({}), which seems to be common in JavaScript, for example, but it means that
with ({) all the brackets are marked as mismatches. Mismatches are no longer
minimized, but the algorithm is simpler and long-distance effects may be
reduced.

Bracket matching and scanning could interact, especially if /* and */ are
regarded as brackets. In most editors, if you type an unmatched /* then the
rest of the file, or at least the part up to the end of the next multiline
comment, is marked up as a comment. But scanning text as a comment could
possibly be delayed until the comment markers match. With /*.../*...*/ which
is technically legal, it is important to match the first /* rather than the
second.

If a user types an unmatched open bracket ( or [ or { at the end of a line,
a simple left-to-right auto-indent algorithm would immediately indent all
subsequent lines. That means the lines after the current one would move in and
out frequently while typing, which doesn't feel good. It would be better to
use a bracket matching algorithm which marked the newly typed open brackets as
mismatched. That means processing each line in the context of the subsequent
lines (as if using a right-to-left algorithm for them).

There are always cases where many lines need to be moved in or out to correct
the indenting. But if a user types curly brackets round a block of lines, the
best outcome might be if the open bracket is marked as a mismatch, and then
the block of lines moved in when the matching close curly bracket is typed.

It would be good if the algorithm consisted of a local phase which operates
on the current line, followed by a possible but relatively rare fix-up phase
between lines.

The usual linear left-to-right version of this is, roughly

    mark all brackets as mismatched
    for each bracket
        if it is a close bracket
            if it matches the most recent mismatched open bracket
                mark both as matched

However, if there are mismatched brackets, the questions arise of (a) how to
make sure the minimum number are marked and (b) which ones to mark if there is
any ambiguity. Consider the following variation:

   when ( is the most recent unmatched bracket and ] is the next close bracket
   if ] matches the second most recent [..(] match them
   else if ( matches the next close bracket (]...) match them

Is two enough? What about the ambiguity of ([)] ?

 only one of a mismatched
pair of brackets (] is marked as a mismatch where reasonably possible, e.g. in
cases (]..) and [..(] only the middle bracket is marked as a mismatch. [Prove
this yields a minimum?]

There are two markings for ([)] where one of the matching pairs is marked as
mismatching. Left-to-right would match up the round brackets.

As well as marking brackets as mismatched, there is a question of which to mark.

    in      l-r    i-o   r-l   o-i
    (()     [()    [()   [()   ([)
    ())     ()]    ()]   ()]   ([)

There is a lot to be said for the o-i algorithm. One, it matches the above
algorithm. Two, it is better for incremental matching, where the outer () might
be a long way away. Three, it is better for multi-line comments: /* /* */.
Since this is 'correct', it would be better to regard the second /* as a
mismatch (unless the second can be discounted by the scanner).

What about when typing? It is good for inserting ( inside (...) because the
most recently typed bracket is marked as a mismatch. Typing ( in front of (...)
gives ([...) which is arguably not intuitive, but provided typing a )
immediately after the new ( doesn't give (][...), all is well.

If ( is typed in the middle of (...), it
would be more natural for the most recently typed bracket to be marked as a
mismatch. If ( is typed in front of (...) then it is also the most recent
bracket that would be most natural.  Suppose not: ([...) then what happens
when ) is typed after ]? Immediately, it is (][...) yet a better match is
possible. On the other hand, [(...) followed by [(]...) works well.

 This is clear. If
you match up brackets using any normal algorithm, the mismatched ones
consist of some 'extra' closers and openers, with all the closers coming before
all the openers.

Current state: string of brackets, some marked red as mismatches. The unmarked
ones are guaranteed to be properly matched. ALSO, in some sense, the number of
mismatched brackets should be 'minimum' and 'optimum'. Minimum = mark
outdenters before indenters.

Operations: insert a bracket, delete a bracket.

Insertion: at any point, insert (. If it 'matches' the next red close bracket,
mark them both as matched, otherwise mark it as mismatched. At any point,
insert ). If it matches the previous red open bracket, mark both matched.

Deletion: at any point, delete black (. Find matching ) and paint red.
Delete black ) similar. Delete red ( or ), do nothing (or check for newly
matched brackets on either side?).

Do you allow brackets on either side of a mismatched bracket to match, e.g. {(}.
Answer yes, because you just typed the (.
