#ifndef _20221228_055431_glyph_box_h
#define _20221228_055431_glyph_box_h

#include "glyph.h"

typedef struct {
	int maxtop, minbot, maxwidth, minleft;
} GlyphBox;

extern void glyphBox_first (GlyphBox *, Glyph *);
extern void glyphBox_next (GlyphBox *, Glyph *);

#endif
