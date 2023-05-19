/* src/glyph.c */

#include "glyph.h"
#include "alloc.h"
#include "gbox.h"

static hb_buffer_t *renderbuf;

void init_glyph () { renderbuf = hb_buffer_create(); }
void term_glyph () { hb_buffer_destroy(renderbuf); }

void glyph_term (glyph_t *g) { alloc_free_puchar(g->bitmap); }

static void glyph_apply_diff (glyph_t *g, glyph_t *a, unsigned top, unsigned left, int n) {
	unsigned y, x, gpd;
	unsigned char *gp, *ap, v;
	
	ap = a->bitmap;
	gp = g->bitmap + g->width*top + left;
	gpd = g->width - a->width;
	
	for (y = 0; y != a->height; y++) {
		for (x = 0; x != a->width; x++) {
			v = *ap >> 1;
			
			if (!n)
				*gp = v;
			else
				if (v > *gp)
					*gp = 0x80 | v - *gp;
				else if (*gp > v)
					*gp = 0x00 | *gp - v;
				else *gp = 0;
			gp++, ap++;
		}
		
		gp += gpd;
	}
}

void glyph_init_diff (glyph_t *g, glyph_t *a, glyph_t *b) {
	gbox_t gbox;
	
	gbox_first(&gbox, a);
	gbox_next(&gbox, b);
	
	g->index = a->index;
	g->top = g->left = 0;
	g->height = gbox.maxtop - gbox.minbot;
	g->width = gbox.maxwidth - gbox.minleft;
	g->bitmap = alloc_alloc_puchar(g->height * g->width);
	memset(g->bitmap, 0x00, sizeof *g->bitmap * g->height * g->width);
	
	glyph_apply_diff(g, a, gbox.maxtop - a->top, a->left - gbox.minleft, 0);
	glyph_apply_diff(g, b, gbox.maxtop - b->top, b->left - gbox.minleft, 1);
}

static void glyph_apply_add (glyph_t *g, glyph_t *a, unsigned top, unsigned left) {
	unsigned y, x, gpd;
	unsigned char *gp, *ap;
	
	ap = a->bitmap;
	gp = g->bitmap + g->width*top + left;
	gpd = g->width - a->width;
	
	for (y = 0; y != a->height; y++) {
		for (x = 0; x != a->width; x++) {
			*gp = *ap + *gp > 0xff ? 0xff : *ap + *gp;
			gp++, ap++;
		}
		
		gp += gpd;
	}
}

static void parts_render (glyph_t parts[], unsigned partslen, FT_Face ftface, hb_glyph_info_t infos[], hb_glyph_position_t positions[]) {
	glyph_t *p;
	FT_Bitmap *bitmap;
	unsigned char *from, *to;
	unsigned i, y, x;
	int xcursor;
	
	xcursor = 0;
	for (i = 0; i != partslen; i++) {
		FT_Load_Glyph(ftface, infos[i].codepoint, FT_LOAD_NO_HINTING|FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL);
		bitmap = &ftface->glyph->bitmap;
		p = &parts[i];
		
//printf("%d,%d %d,%d %dx%d\n", positions[i].y_offset, positions[i].x_offset, ftFace->glyph->bitmap_top, ftFace->glyph->bitmap_left, bitmap->rows, bitmap->width);
		p->index = infos[i].codepoint;
		p->top = ftface->glyph->bitmap_top + positions[i].y_offset;
		p->left = xcursor + ftface->glyph->bitmap_left + positions[i].x_offset;
		p->height = bitmap->rows;
		p->width = bitmap->width;
		p->bitmap = alloc_alloc_puchar(p->height * p->width);
		
		from = bitmap->buffer;
		to = p->bitmap;
		for (y = 0; y != bitmap->rows; y++) {
			for (x = 0; x != bitmap->width; x++)
				*to++ = from[x];
			from += bitmap->pitch;
		}
		
		xcursor += positions[i].x_advance;
	}
}

static void parts_combine (glyph_t parts[], unsigned partslen, glyph_t *g) {
	gbox_t gbox;
	glyph_t *p;
	unsigned i;
	
	gbox_first(&gbox, &parts[0]);
	for (i = 1; i != partslen; i++)
		gbox_next(&gbox, &parts[i]);
	
	g->index = parts[0].index;
	g->top = gbox.maxtop;
	g->left = gbox.minleft;
	g->height = gbox.maxtop - gbox.minbot;
	g->width = gbox.maxwidth - gbox.minleft;
	g->bitmap = alloc_alloc_puchar(g->height * g->width);
	memset(g->bitmap, 0x00, sizeof *g->bitmap * g->height * g->width);
	
	for (i = 0; i != partslen; i++) {
		p = &parts[i];
		glyph_apply_add(g, p, gbox.maxtop - p->top, p->left - gbox.minleft);
	}
}

void glyph_init_render (glyph_t *g, hb_font_t *hbfont, FT_Face ftface, hb_codepoint_t cp, hb_feature_t *ft) {
	glyph_t parts[16];
	unsigned partslen;
	hb_glyph_info_t *infos;
	hb_glyph_position_t *positions;
	unsigned i;
	
	hb_buffer_reset(renderbuf);
	
	hb_buffer_set_direction(renderbuf, HB_DIRECTION_LTR);
	hb_buffer_set_content_type(renderbuf, HB_BUFFER_CONTENT_TYPE_UNICODE);
	hb_buffer_add(renderbuf, cp, 0);
	hb_shape(hbfont, renderbuf, ft, !ft ? 0 : 1);
	
	infos = hb_buffer_get_glyph_infos(renderbuf, &partslen);
	/*
	if (!infoslen || !infos[0].codepoint)
		s->keylen = 0;
	*/
	positions = hb_buffer_get_glyph_positions(renderbuf, &partslen);
	parts_render(parts, partslen, ftface, infos, positions);
	parts_combine(parts, partslen, g);
	for (i = 0; i != partslen; i++)
		alloc_free_puchar(parts[i].bitmap);
}
