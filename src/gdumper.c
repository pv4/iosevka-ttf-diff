/* src/gdumper.c */

#include "gdumper.h"
#include "png.h"
#include "lib.h"

static pngrow_t pngrow;

void init_gdumper () { pngrow_init(&pngrow, 8192); }
void term_gdumper () { pngrow_term(&pngrow); }

void gdumper_ascii (glyph_t *g) {
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

void gdumper_ascii_diff (glyph_t *g, glyph_t *a, glyph_t *b) {
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

void gdumper_png_diff (glyph_t *g, glyph_t *a, glyph_t *b, const char *path) {
	png_t png;
	unsigned y, x, i;
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
	
	png_init(&png, path,
		g->height, g->width + 15 + aleft+a->width + 15 + bleft+b->width);
	
	for (y = 0; y != g->height; y++) {
		pngrow_reset(&pngrow);
		
		for (x = 0; x != g->width; x++) {
			d = *gp & 0x7f;
			
			if (!d) pngrow_add(&pngrow, 0xff, 0xff, 0xff);
			else if (!(*gp & 0x80)) pngrow_add(&pngrow, 0xbf-d, 0x7f-d, 0x7f-d);
			else pngrow_add(&pngrow, 0x7f-d, 0xbf-d, 0x7f-d);
			gp++;
		}
		for (i = 0; i != 15; i++)
			pngrow_add(&pngrow, 0x00, 0x40, 0x80);
		if (y < atop || y >= atop + a->height)
			for (x = 0; x != aleft + a->width; x++)
				pngrow_add(&pngrow, 0xff, 0xff, 0xff);
		else {
			for (x = 0; x != aleft; x++)
				pngrow_add(&pngrow, 0xff, 0xff, 0xff);
			for (x = 0; x != a->width; x++) {
				pngrow_add(&pngrow, 0xff-*ap, 0xff-*ap, 0xff-*ap);
				ap++;
			}
		}
		for (i = 0; i != 15; i++)
			pngrow_add(&pngrow, 0x00, 0x40, 0x80);
		if (y < btop || y >= btop + b->height)
			for (x = 0; x != bleft + b->width; x++)
				pngrow_add(&pngrow, 0xff, 0xff, 0xff);
		else {
			for (x = 0; x != bleft; x++)
				pngrow_add(&pngrow, 0xff, 0xff, 0xff);
			for (x = 0; x != b->width; x++) {
				pngrow_add(&pngrow, 0xff-*bp, 0xff-*bp, 0xff-*bp);
				bp++;
			}
		}
		
		png_add(&png, &pngrow);
	}
	
	png_term(&png);
}
