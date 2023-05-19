/* src/png.c */

#include "png.h"

#include "alloc.h"

void png_init (png_t *png, const char *path, unsigned height, unsigned width) {
	png->f = fopen(path, "wb");
	png->png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png->info = png_create_info_struct(png->png);
	png_init_io(png->png, png->f);
	png_set_compression_level(png->png, Z_BEST_COMPRESSION);
	png_set_IHDR(png->png, png->info,
		(png_uint_32)width, (png_uint_32)height, 8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png->png, png->info);
}
void png_term (png_t *png) {
	png_write_end(png->png, png->info);
	//todo: no png_destroy_info_struct?
	png_destroy_write_struct(&png->png, &png->info);
	fclose(png->f);
}

void png_add (png_t *png, pngrow_t *row) { png_write_row(png->png, row->data); }

void pngrow_init (pngrow_t *row, unsigned len) {
	row->data = alloc_alloc_puchar(3 * len);
	row->i = 0;
}
void pngrow_term (pngrow_t *row) { alloc_free_puchar(row->data); }

void pngrow_reset (pngrow_t *row) { row->i = 0; }
void pngrow_add (pngrow_t *row, unsigned char r, unsigned char g, unsigned char b) {
	unsigned char *p = row->data + 3*row->i++;
	*p++ = r;
	*p++ = g;
	*p++ = b;
}
