# Overall dependencies:
# control -> view/model/filer

filer: utf16 list-of-strings (internalize?)
settings: filer string(split/normalize)
array: not (yet) used
style: (arrange NOT to share with view, but DO share with filer/settings/theme)
utf8

op?: broadcast op to all components.
	Fix flag (affects undo), Maybe similar to Multi
	a) insert (before)
	b) delete (before)
	c) move cursor
	d) drag cursor
	e) deselect
	f) point (leave old selection intact, make new cursor)
	g) fix indent, fix trailers (don't store for undo)

Atomic ops:

Insert before.
Delete before (delete right is move right + delete).
   delete selection is move to right end, deselecting, then delete.
Move. (Move to selection = deselect. Any move is deselect.)
Drag.
Cycle. (Change cursors cyclically. This a cursor move? Undo is reverse cycle?)
   Where do we get insertion/deletion text? Or does Text drive Undo?
   Start and drag could be flags. Should Undo be a flag?
Undo
End edit.

Have op structure, gather data from components. Therefore delegate in sequence,
so the right info has been gathered.
