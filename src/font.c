// The Snipe editor is free and open source, see licence.txt.
#include "font.h"
#include "file.h"
#include <stdlib.h>
#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// Freetype is capable of producing high quality anti-aliased text. However, it
// is extremely easy to introduce unwanted extra anti-aliasing when rendering,
// by not mapping the generated pixels sufficiently precisely to screen pixels.
// This can happen e.g. as a result of floating point rounding, and it lowers
// the visual quality. It appears that SDL + SDL_ttf can suffer from this at
// small font sizes. The approach taken here is to produce a strip image for a
// page of 256 characters, expanding each character image to the maximum width
// (and ascent and descent) for the characters on the page. Each individual
// character then takes up 1/256 of the image, and 1.0/256.0 is an exact
// floating point number.

// This precomputation of a strip image means that Freetype's ability to do
// sub-pixel positioning isn't being used, but that's OK since the aim is to
// produce a monospaced visual effect. However, even a notionally monospaced
// font isn't truly monospaced, so the advance values for the individual
// characters are recorded and used, rather than just placing characters in a
// grid. This also means that non-monospaced fonts should look reasonable.

struct page {
    font *f;
    int fontSize, start, ascent;
    int height, width;
    unsigned char *image;
    uint32_t textureId;
    short advances[256];
    struct page *next, *back;
};

struct font {
    FT_Library library;
    FT_Face face;
    struct page *pages;
};

// If a freetype function fails, print a message and exit.
static void nz(int r) {
    if (r == 0) return;
    printf("Freetype error 0x%x (see fterrdef.h online)\n", r);
    exit(1);
}

font *newFont(char *file) {
    font *f = malloc(sizeof(font));
    nz(FT_Init_FreeType(&f->library));
    f->pages = NULL;
    char *path = resourcePath("", file, "");
    nz(FT_New_Face(f->library, path, 0, &f->face));
    free(path);
    return f;
}

void freeFont(font *f) {
    FT_Done_Face(f->face);
    FT_Done_FreeType(f->library);
    while (f->pages != NULL) {
        struct page *p = f->pages;
        f->pages = f->pages->next;
        if (p->image != NULL) free(p->image);
        free(p);
    }
    free(f);
}

// Find measurements for a page. Freetype metrics have y upwards, freetype
// bitmaps have y downwards, and the images produced have y upwards.
static void measure(FT_Face face, struct page *p) {
    FT_GlyphSlot g = face->glyph;
    FT_Bitmap *b = &g->bitmap;
    int maxAscent = 0, maxDescent = 0, maxWidth = 0;
    for (int i = 0; i < 256; i++) {
        nz(FT_Load_Char(face, p->start + i, FT_LOAD_RENDER));
        int ascent = g->bitmap_top + 1;
        int descent = b->rows - g->bitmap_top;
        int width = g->bitmap_left + b->width;
        int advance = g->advance.x / 64;
        if (ascent > maxAscent) maxAscent = ascent;
        if (descent > maxDescent) maxDescent = descent;
        if (width > maxWidth) maxWidth = width;
        p->advances[i] = advance;
    }
    p->ascent = maxAscent;
    p->height = maxAscent + maxDescent;
    p->width = 256 * maxWidth;
}

// Create the image for a page. Freetype metrics have y upwards, freetype
// bitmaps have y downwards, and the images produced have y upwards.
static void buildImage(font *f, struct page *p) {
    p->image = calloc(p->width * p->height, 4);
    FT_GlyphSlot g = f->face->glyph;
    FT_Bitmap *b = &g->bitmap;
    int charWidth = p->width / 256;
    for (int i = 0; i < 256; i++) {
        nz(FT_Load_Char(f->face, p->start + i, FT_LOAD_RENDER));
        for (int y = 0; y < b->rows; y++) {
            int imgy = p->height - p->ascent + g->bitmap_top - y;
            for (int x = 0; x < b->width; x++) {
                int imgx = x + g->bitmap_left;
                int n = (imgy * p->width + (i * charWidth + imgx)) * 4;
                if ((y == 0 || y == b->rows-1) && (x == 0 || x == b->width-1)) {
                    assert( n < p->width * p->height * 4);
                }
                p->image[n] = 255;
                p->image[n+1] = 255;
                p->image[n+2] = 255;
                p->image[n+3] = b->buffer[y * b->width + x];
            }
        }
    }
}

// Find or create a page.
page *getPage(font *f, int size, int start) {
    assert(start % 256 == 0);
    for (struct page *p = f->pages; p != NULL; p = p->next) {
        if (p->fontSize == size && p->start == start) return p;
    }
    struct page *p = malloc(sizeof(struct page));
    p->f = f;
    p->fontSize = size;
    p->start = start;
    nz(FT_Set_Pixel_Sizes(f->face, 0, p->fontSize));
    measure(f->face, p);
    buildImage(f, p);
    p->back = NULL;
    p->next = f->pages;
    if (p->next != NULL) p->next->back = p;
    f->pages = p;
    return p;
}

int pageWidth(page *p) { return p->width; }
int pageHeight(page *p) { return p->height; }
unsigned char *pageImage(page *p) { return p->image; }
extern inline int charAdvance(page *p, int ch) { return p->advances[ch % 256]; }

#ifdef test_font

int main(int n, char const *args[]) {
    setbuf(stdout, NULL);
    findResources(args[0]);
    font *f = newFont("files/DejaVuSansMono.ttf");
    page *p = getPage(f, 20, 0);
    assert(p->height == 25);
    assert(p->ascent == 20);
    assert(p->width == 3328);
    freeFont(f);
    freeResources();
    printf("Font module OK\n");
    return 0;
}

#endif
