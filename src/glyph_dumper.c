#include "glyph_dumper.h"
#include "lib.h"

static unsigned char pngbuf[3*8192];

void GlyphDumper_ascii (Glyph *g) {
	unsigned y, x;
	unsigned char *p;
	
	p = g->bitmap;
	
	for (y = 0; y != g->height; y++) {
		for (x = 0; x != g->width; x++) {
			printf("%s", *p == 0xff ? "##" : *p == 0x00 ? ".." : "++");
			p++;
		}
		printf("\n");
	}
}

void GlyphDumper_asciiDiff (Glyph *g, Glyph *a, Glyph *b) {
	unsigned y, x;
	int maxtop, minleft, atop, btop, aleft, bleft;
	unsigned char *gp, *ap, *bp, d;
	
	maxtop = max(0, max(a->top, b->top));
	minleft = min(a->left, b->left);
	
	atop = maxtop - a->top;
	btop = maxtop - b->top;
	aleft = a->left - minleft;
	bleft = b->left - minleft;
	
	gp = g->bitmap;
	ap = a->bitmap;
	bp = b->bitmap;
	
	for (y = 0; y != g->height; y++) {
		for (x = 0; x != g->width; x++) {
			d = *gp & 0x7f;
			
			printf("%s", d == 0x7f ? "##" : d == 0x00 ? ".." : "++");
			gp++;
		}
		printf("    ");
		if (y < atop || y >= atop + a->height)
			for (x = 0; x != aleft + a->width; x++)
				printf("..");
		else {
			for (x = 0; x != aleft; x++)
				printf("..");
			for (x = 0; x != a->width; x++) {
				printf("%s", *ap == 0xff ? "##" : *ap == 0x00 ? ".." : "++");
				ap++;
			}
		}
		printf("    ");
		if (y < btop || y >= btop + b->height)
			for (x = 0; x != bleft + b->width; x++)
				printf("..");
		else {
			for (x = 0; x != bleft; x++)
				printf("..");
			for (x = 0; x != b->width; x++) {
				printf("%s", *bp == 0xff ? "##" : *bp == 0x00 ? ".." : "++");
				bp++;
			}
		}
		printf("\n");
	}
}

static inline void pngbuf_reset (unsigned char *pngbuf, unsigned *n) { *n = 0; }
static inline void pngbuf_add (unsigned char *pngbuf, unsigned *n, unsigned char r, unsigned char g, unsigned char b) {
	pngbuf[(*n)++] = r;
	pngbuf[(*n)++] = g;
	pngbuf[(*n)++] = b;
}

void GlyphDumper_pngDiff (Glyph *g, Glyph *a, Glyph *b, const char *path) {
	png_structp png;
	png_infop info;
	FILE *file;
	unsigned y, x, n, i;
	int maxtop, minleft, atop, btop, aleft, bleft;
	unsigned char *gp, *ap, *bp, d;
	
	maxtop = max(0, max(a->top, b->top));
	minleft = min(a->left, b->left);
	
	atop = maxtop - a->top;
	btop = maxtop - b->top;
	aleft = a->left - minleft;
	bleft = b->left - minleft;
	
	gp = g->bitmap;
	ap = a->bitmap;
	bp = b->bitmap;
	
	file = fopen(path, "wb");
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info = png_create_info_struct(png);
	png_init_io(png, file);
	png_set_compression_level(png, Z_BEST_COMPRESSION);
	png_set_IHDR(png, info,
		g->width + 15 + aleft+a->width + 15 + bleft+b->width, g->height,
		8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);
	
	for (y = 0; y != g->height; y++) {
		pngbuf_reset(pngbuf, &n);
		
		for (x = 0; x != g->width; x++) {
			d = *gp & 0x7f;
			
			if (!d) pngbuf_add(pngbuf, &n, 0xff, 0xff, 0xff);
			else if (!(*gp & 0x80)) pngbuf_add(pngbuf, &n, 0xbf-d, 0x7f-d, 0x7f-d);
			else pngbuf_add(pngbuf, &n, 0x7f-d, 0xbf-d, 0x7f-d);
			gp++;
		}
		for (i = 0; i != 15; i++)
			pngbuf_add(pngbuf, &n, 0x00, 0x40, 0x80);
		if (y < atop || y >= atop + a->height)
			for (x = 0; x != aleft + a->width; x++)
				pngbuf_add(pngbuf, &n, 0xff, 0xff, 0xff);
		else {
			for (x = 0; x != aleft; x++)
				pngbuf_add(pngbuf, &n, 0xff, 0xff, 0xff);
			for (x = 0; x != a->width; x++) {
				pngbuf_add(pngbuf, &n, 0xff-*ap, 0xff-*ap, 0xff-*ap);
				ap++;
			}
		}
		for (i = 0; i != 15; i++)
			pngbuf_add(pngbuf, &n, 0x00, 0x40, 0x80);
		if (y < btop || y >= btop + b->height)
			for (x = 0; x != bleft + b->width; x++)
				pngbuf_add(pngbuf, &n, 0xff, 0xff, 0xff);
		else {
			for (x = 0; x != bleft; x++)
				pngbuf_add(pngbuf, &n, 0xff, 0xff, 0xff);
			for (x = 0; x != b->width; x++) {
				pngbuf_add(pngbuf, &n, 0xff-*bp, 0xff-*bp, 0xff-*bp);
				bp++;
			}
		}
		
		png_write_row(png, pngbuf);
	}
	
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);
	fclose(file);
}
