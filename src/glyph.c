#include "glyph.h"
#include "alloc.h"
#include "glyph_box.h"
#include "lib.h"

static hb_buffer_t *buf;

void init_glyph () { buf = hb_buffer_create(); }
void term_glyph () { hb_buffer_destroy(buf); }

void glyph_term (glyph_t *g) { alloc_free_puchar(g->bitmap); }

static void _glyph_apply_diff (glyph_t *g, glyph_t *a, unsigned top, unsigned left, int n) {
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
	glyph_box_t gbox;
	
	glyph_box_first(&gbox, a);
	glyph_box_next(&gbox, b);
	
	g->index = a->index;
	g->top = g->left = 0;
	g->height = gbox.maxtop - gbox.minbot;
	g->width = gbox.maxwidth - gbox.minleft;
	g->bitmap = alloc_alloc_puchar(g->height * g->width);
	memset(g->bitmap, 0x00, sizeof *g->bitmap * g->height * g->width);
	
	_glyph_apply_diff(g, a, gbox.maxtop - a->top, a->left - gbox.minleft, 0);
	_glyph_apply_diff(g, b, gbox.maxtop - b->top, b->left - gbox.minleft, 1);
}

static void buf_shape (hb_buffer_t *buf, hb_font_t *hbfont, hb_codepoint_t cp, hb_feature_t *feat) {
	hb_buffer_reset(buf);
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_content_type(buf, HB_BUFFER_CONTENT_TYPE_UNICODE);
	hb_buffer_add(buf, cp, 0);
	
	hb_shape(hbfont, buf, feat, !feat ? 0 : 1);
/*
	unsigned i, nglyphs;
	
	hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buf, &nglyphs);
	hb_glyph_position_t *poses = hb_buffer_get_glyph_positions(buf, &nglyphs);
	for (i = 0; i != nglyphs; i++)
		printf("%d %u (%d,%d) adv (%d, %d)\n",
			infos[i].codepoint, infos[i].cluster,
			poses[i].y_offset, poses[i].x_offset,
			poses[i].y_advance, poses[i].x_advance);
*/
}

static void _glyph_apply_add (glyph_t *g, glyph_t *a, unsigned top, unsigned left) {
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
	glyph_box_t gbox;
	glyph_t *p;
	unsigned i;
	
	glyph_box_first(&gbox, &parts[0]);
	
	for (i = 1; i != partslen; i++)
		glyph_box_next(&gbox, &parts[i]);
	
	g->index = parts[0].index;
	g->top = gbox.maxtop;
	g->left = gbox.minleft;
	g->height = gbox.maxtop - gbox.minbot;
	g->width = gbox.maxwidth - gbox.minleft;
	g->bitmap = alloc_alloc_puchar(g->height * g->width);
	memset(g->bitmap, 0x00, sizeof *g->bitmap * g->height * g->width);
	
	for (i = 0; i != partslen; i++) {
		p = &parts[i];
		_glyph_apply_add(g, p, gbox.maxtop - p->top, p->left - gbox.minleft);
	}
}

void glyph_init_render (glyph_t *g, hb_font_t *hbfont, FT_Face ftface, gik_t *gik, hb_codepoint_t cp, hb_feature_t *feat) {
	glyph_t parts[16];
	unsigned partslen;
	hb_glyph_info_t *infos;
	hb_glyph_position_t *positions;
	unsigned i;
	
	buf_shape(buf, hbfont, cp, feat);
	
	infos = hb_buffer_get_glyph_infos(buf, &partslen);
	if (!partslen || !infos[0].codepoint) {
		g->bitmap = NULL;
		gik->len = 0;
		return;
	}
	positions = hb_buffer_get_glyph_positions(buf, &partslen);
	
	parts_render(parts, partslen, ftface, infos, positions);
	parts_combine(parts, partslen, g);
	
	for (i = 0; i != partslen; i++)
		alloc_free_puchar(parts[i].bitmap);
	
	hb_buffer_serialize_glyphs(buf, 0, -1, gik->val, sizeof gik->val / sizeof *gik->val, &gik->len, hbfont, HB_BUFFER_SERIALIZE_FORMAT_TEXT, HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES);
}
