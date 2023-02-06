#ifndef _20221228_055431_glyph_h
#define _20221228_055431_glyph_h

#include "gik.h"
#include "main.h"

typedef struct {
	unsigned char *bitmap;
	int height, width, top, left;
	unsigned index;
} Glyph;

extern void Glyph_init ();
extern void Glyph_term ();

extern void glyph_initRender (Glyph *g, hb_font_t *hbFont, FT_Face ftFace, Gik *gik, hb_codepoint_t cp, hb_feature_t *feat);
extern void glyph_initDiff (Glyph *g, Glyph *a, Glyph *b);
extern void glyph_term (Glyph *g);

#endif
