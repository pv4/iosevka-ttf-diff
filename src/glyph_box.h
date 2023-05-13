#ifndef h_20221228_055431_glyph_box
#define h_20221228_055431_glyph_box

#include "glyph.h"

typedef struct {
	int maxtop, minbot, maxwidth, minleft;
} glyph_box_t;

extern void glyph_box_first (glyph_box_t *b, glyph_t *g);
extern void glyph_box_next (glyph_box_t *b, glyph_t *g);

#endif
