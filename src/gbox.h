#ifndef h_20221228_055431_gbox
#define h_20221228_055431_gbox

#include "glyph.h"

typedef struct {
	int maxtop, minbot, maxwidth, minleft;
} gbox_t;

extern void gbox_first (gbox_t *b, glyph_t *g);
extern void gbox_next (gbox_t *b, glyph_t *g);

#endif
