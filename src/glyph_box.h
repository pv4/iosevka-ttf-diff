#ifndef _20221228_055431_glyph_box_h
#define _20221228_055431_glyph_box_h

#include "glyph.h"

typedef struct {
	int maxtop, minbot, maxwidth, minleft;
} glyph_box_t;

extern void glyph_box_first (glyph_box_t *b, glyph_t *g);
extern void glyph_box_next (glyph_box_t *b, glyph_t *g);

#endif
