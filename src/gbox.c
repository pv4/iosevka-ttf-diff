#include "gbox.h"
#include "lib.h"

void gbox_first (gbox_t *b, glyph_t *g) {
	b->maxtop = max(0, g->top);
	b->minbot = min(0, g->top - g->height);
	b->maxwidth = g->left + g->width;
	b->minleft = g->left;
}

void gbox_next (gbox_t *b, glyph_t *g) {
	if (g->top > b->maxtop) b->maxtop = g->top;
	if (g->top - g->height < b->minbot) b->minbot = g->top - g->height;
	if (g->left + g->width > b->maxwidth) b->maxwidth = g->left + g->width;
	if (g->left < b->minleft) b->minleft = g->left;
}
