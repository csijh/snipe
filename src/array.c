// Snipe editor. Free and open source, see licence.txt.

// Pointer-aligned header, prefixed to arrays.
struct header { int length, max, unit; void *align[]; };
typedef struct header header;

void *newArray(int unit) {
    int max0 = 1;
    header *h = malloc(sizeof(header) + max0 * unit);
    *h = (header) { .length = 0, .max = max0, .unit = unit };
    return h + 1;
}

int length(void *a) {
    header *h = (header *) a - 1;
    return h->length;
}

void *ensure(void *a, int d) {
    header *h = (header *) a - 1;
    int n = h->length + d;
    if (n <= h->max) return a;
    while (n > h->max) h->max = 2 * h->max;
    h = realloc(h, sizeof(header) + h->max * h->unit);
    return h + 1;
}

void *adjust(void *a, int d) {
    a = ensure(a,d);
    header *h = (header *) a - 1;
    h->length += d;
    if (h->length < 0) h->length = 0;
    return h + 1;
}


