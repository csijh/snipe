// Snipe editor. Free and open source, see licence.txt.

// The places before the gap are normal
// indexes. The places after the gap are relative to the end of the text, so
// that they remain stable across insertions and deletions at the cursor. Text
// insertions and deletions are monitored, to track the size of the text, and
// make any necessary but usually minor adjustments to the gap position.

// The gap buffer is 0..low..high..max, and the text is 0..end.
struct places Places { int low, high, max, end; int *data; };

// Create or free a places object.
Places *newPlaces() {
    int max0 = 2;
    Places *ps = malloc(sizeof(Places));
    int *data = malloc(max0 * sizeof(int));
    *ps = (Places) { .low=0, .high=max0, .max=max0, .data=data };
    return ps;
}

void freePlaces(Places *ps) {
    free(ps->data);
    free(ps);
}

int sizeP(Places *ps) {
    return ps->low + ps->max - ps->high;
}

// Move the gap to place p. Change signs of places across the gap.
static void move(Places *ps, int p) {
    while (ps->low > 0 && ps->data[ps->low-1] > p) {
        ps->data[--ps->high] = ps->data[--ps->low] - ps->end;
    }
    while (ps->high < ps->max && ps->end + ps->data[ps->high] <= p) {
        ps->data[ps->low++] = ps->end + ps->data[ps->high];
    }
}

void editP(Places *ps, int p, int n) {
    move(ps, p);
    if (n < 0) {
        int limit = p - n + 1;
        if (ps->low > 0 && ps->data[ps->low-1] == p) limit = p - n;
        while (ps->high < ps->max && ps->end + ps->data[ps->high] <= limit) {
            ps->high++;
        }
    }
    ps->end += n;
}

// Get the i'th place (non-negative).
int getP(Places *ps);

// Set the i'th Place.
int setP(Places *ps);

// Insert a place at position i.
void insertP(Places *ps, int i, int p);

// Delete the place at position i.
void deleteP(Places *ps, int i);

// ============================

void freeBytes(Bytes *ps) {
}

int size(Bytes *ps) {
}

char get(Bytes *ps, int i) {
    if (i < ps->low) return ps->data[i];
    return ps->data[i + ps->high - ps->low];
}

void set(Bytes *ps, int i, char b) {
    if (i < ps->low) ps->data[i] = b;
    else ps->data[i + ps->high - ps->low] = b;
}

void move(Bytes *ps, int cursor) {
    int low = ps->low, high = ps->high;
    char *data = ps->data;
    if (cursor < low) {
        memmove(data + cursor + high - low, data + cursor, low - cursor);
    }
    else if (cursor > low) {
        memmove(data + low, data + high, cursor - low);
    }
    ps->low = cursor;
    ps->high = cursor + high - low;
}

static void ensure(Bytes *ps, int extra) {
    int low = ps->low, high = ps->high, max = ps->max;
    char *data = ps->data;
    int new = max;
    while (new < low + max - high + extra) new = new * 3 / 2;
    data = realloc(data, new);
    if (high < max) {
        memmove(data + high + new - max, data + high, max - high);
    }
    ps->high = high + new - max;
    ps->max = new;
    ps->data = data;
}

void insert(Bytes *ps, int i, char *s, int n) {
    if (ps->high - ps->low < n) ensure(ps, n);
    move(ps, i);
    memcpy(ps->data + ps->low, s, n);
    ps->low += n;
}

void replace(Bytes *ps, int i, char *s, int n) {
    move(ps, i + n);
    memcpy(ps->data + i, s, n);
}

// Copy or copy-and-delete n bytes from index i into s.
void copy(Bytes *ps, int i, char *s, int n) {
    move(ps, i + n);
    memcpy(s, ps->data + i, n);
}

void delete(Bytes *ps, int i, char *s, int n) {
    move(ps, i + n);
    memcpy(s, ps->data + i, n);
    ps->low = i;
}

int cursor(Bytes *ps) {
    return ps->low;
}

// Load a file (deleting any previous content) or save the content into a file.
void load(Bytes *ps, char *path);
void save(Bytes *ps, char *path);

// ---------- Testing ----------------------------------------------------------
#ifdef bytesTest

// Check that a Bytes object matches a string.
static bool eq(Bytes *ps, char *s) {
    if (strlen(s) != ps->max) return false;
    for (int i = 0; i < ps->max; i++) {
        if (ps->low <= i && i < ps->high) { if (s[i] != '-') return false; }
        else if (s[i] != ps->data[i]) return false;
    }
    return true;
}

// Test gap buffer with char items.
static void test() {
    Bytes *ps = newBytes();
    ensure(ps, 10);
    assert(eq(ps, "-------------"));
    insert(ps, 0, "abcde", 5);
    assert(eq(ps, "abcde--------"));
    move(ps, 2);
    assert(eq(ps, "ab--------cde"));
    char out[10];
    delete(ps, 1, out, 1);
    assert(eq(ps, "a---------cde"));
    assert(out[0] == 'b');
    ensure(ps, 14);
    assert(eq(ps, "a---------------cde"));
    move(ps,3);
    assert(eq(ps, "acd---------------e"));
    insert(ps, 3, "xyz", 3);
    assert(eq(ps, "acdxyz------------e"));
    insert(ps, 1, "uvw", 3);
    assert(eq(ps, "auvw---------cdxyze"));
    freeBytes(ps);
}

int main() {
    test();
    printf("Bytes module OK\n");
}

#endif
