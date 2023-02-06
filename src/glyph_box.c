#include "glyph_box.h"
#include "lib.h"

void glyphBox_first (GlyphBox *b, Glyph *g) {
	b->maxtop = max(0, g->top);
	b->minbot = min(0, g->top - g->height);
	b->maxwidth = g->left + g->width;
	b->minleft = g->left;
}

void glyphBox_next (GlyphBox *b, Glyph *g) {
	if (g->top > b->maxtop) b->maxtop = g->top;
	if (g->top - g->height < b->minbot) b->minbot = g->top - g->height;
	if (g->left + g->width > b->maxwidth) b->maxwidth = g->left + g->width;
	if (g->left < b->minleft) b->minleft = g->left;
}
