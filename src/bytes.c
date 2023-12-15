// Snipe editor. Free and open source, see licence.txt.
#include "bytes.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

struct bytes { int low, high, max; char *data; };

Bytes *newBytes() {
    int max0 = 2;
    Bytes *bs = malloc(sizeof(Bytes));
    char *data = malloc(max0);
    *bs = (Bytes) { .low=0, .high=max0, .max=max0, .data=data };
    return bs;
}

void freeBytes(Bytes *bs) {
    free(bs->data);
    free(bs);
}

int size(Bytes *bs) {
    return bs->low + bs->max - bs->high;
}

char get(Bytes *bs, int i) {
    if (i < bs->low) return bs->data[i];
    return bs->data[i + bs->high - bs->low];
}

void set(Bytes *bs, int i, char b) {
    if (i < bs->low) bs->data[i] = b;
    else bs->data[i + bs->high - bs->low] = b;
}

void move(Bytes *bs, int cursor) {
    int low = bs->low, high = bs->high;
    char *data = bs->data;
    if (cursor < low) {
        memmove(data + cursor + high - low, data + cursor, low - cursor);
    }
    else if (cursor > low) {
        memmove(data + low, data + high, cursor - low);
    }
    bs->low = cursor;
    bs->high = cursor + high - low;
}

static void ensure(Bytes *bs, int extra) {
    int low = bs->low, high = bs->high, max = bs->max;
    char *data = bs->data;
    int new = max;
    while (new < low + max - high + extra) new = new * 3 / 2;
    data = realloc(data, new);
    if (high < max) {
        memmove(data + high + new - max, data + high, max - high);
    }
    bs->high = high + new - max;
    bs->max = new;
    bs->data = data;
}

void insert(Bytes *bs, int i, char *s, int n) {
    if (bs->high - bs->low < n) ensure(bs, n);
    move(bs, i);
    memcpy(bs->data + bs->low, s, n);
    bs->low += n;
}

void replace(Bytes *bs, int i, char *s, int n) {
    move(bs, i + n);
    memcpy(bs->data + i, s, n);
}

// Copy or copy-and-delete n bytes from index i into s.
void copy(Bytes *bs, int i, char *s, int n) {
    move(bs, i + n);
    memcpy(s, bs->data + i, n);
}

void delete(Bytes *bs, int i, char *s, int n) {
    move(bs, i + n);
    memcpy(s, bs->data + i, n);
    bs->low = i;
}

int cursor(Bytes *bs) {
    return bs->low;
}

// Load a file (deleting any previous content) or save the content into a file.
void load(Bytes *bs, char *path);
void save(Bytes *bs, char *path);

// ---------- Testing ----------------------------------------------------------
#ifdef bytesTest

// Check that a Bytes object matches a string.
static bool eq(Bytes *bs, char *s) {
    if (strlen(s) != bs->max) return false;
    for (int i = 0; i < bs->max; i++) {
        if (bs->low <= i && i < bs->high) { if (s[i] != '-') return false; }
        else if (s[i] != bs->data[i]) return false;
    }
    return true;
}

// Test gap buffer with char items.
static void test() {
    Bytes *bs = newBytes();
    ensure(bs, 10);
    assert(eq(bs, "-------------"));
    insert(bs, 0, "abcde", 5);
    assert(eq(bs, "abcde--------"));
    move(bs, 2);
    assert(eq(bs, "ab--------cde"));
    char out[10];
    delete(bs, 1, out, 1);
    assert(eq(bs, "a---------cde"));
    assert(out[0] == 'b');
    ensure(bs, 14);
    assert(eq(bs, "a---------------cde"));
    move(bs,3);
    assert(eq(bs, "acd---------------e"));
    insert(bs, 3, "xyz", 3);
    assert(eq(bs, "acdxyz------------e"));
    insert(bs, 1, "uvw", 3);
    assert(eq(bs, "auvw---------cdxyze"));
    freeBytes(bs);
}

int main() {
    test();
    printf("Bytes module OK\n");
}

#endif
