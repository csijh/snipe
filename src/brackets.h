// Could be opaque.
struct bracket { int site; int partner; };

// Custom gap buffer.
struct brackets;

// Contains all long-distance brackets, and all brackets for current line.
// Site is index into types array.
// Partner contains matches, and doubles as stack.
// Where is the stack top held? Search (#c,#o) or special or separate?
// Current line must be cursor line.
// Operations:
//    Make next/prev line current.
//    Remove then replace all entries on current line.

// Case study: compile.c
// Bytes: 40840  (text and types 81680)
// Lines: 5938   (boundaries 23752)
// Brackets: (733+203+166)*2*8 = (brackets structure bytes: 17623)
