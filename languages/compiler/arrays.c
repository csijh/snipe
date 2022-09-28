// How about:

// This structure precedes flexible arrays.
typedef struct array array;
struct array { array **owner; int length, max; };

array *newA() {
    return a + 1;
}

// Resize an array, both locally and in its owning object.
array *resizeA(array *a, int n, int unit) {
    a = a - 1;
    *(a->owner) = a + 1;
    return a + 1;
}

// --------- String store -----------

char *newStrings() { return newA(); }

int lengthS(char *strings) { return ((array *)strings)->length; }

char *resizeS(char *strings, int n) { return resize((array *)strings, n, 1); }

// --------- Lists of text offsets --------------

// A text object is an offset in the string store.
typedef struct { int offset; } text;

// List of texts.
text *newT() {}
int lengthT(text *ts) {}
text *resizeT() {}
