/* src/png.h */

#ifndef h_20221228_055431_png
#define h_20221228_055431_png

#include <png.h>
#include <zlib.h>
#include <stdio.h>

typedef struct {
	png_structp png;
	png_infop info;
	FILE *f;
} png_t;

typedef struct {
	unsigned char *data;
	unsigned i;
} pngrow_t;

extern void png_init (png_t *png, const char *fname, unsigned height, unsigned width);
extern void png_add (png_t *png, pngrow_t *row);
extern void png_term (png_t *png);

extern void pngrow_init (pngrow_t *row, unsigned len);
extern void pngrow_term (pngrow_t *row);
extern void pngrow_reset (pngrow_t *row);
extern void pngrow_add (pngrow_t *row, unsigned char r, unsigned char g, unsigned char b);

#endif
