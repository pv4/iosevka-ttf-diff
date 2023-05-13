#ifndef h_20221228_055431_glyph
#define h_20221228_055431_glyph

#include "gik.h"
#include "main.h"

typedef struct {
	unsigned char *bitmap;
	int height, width, top, left;
	unsigned index;
} glyph_t;

extern void init_glyph ();
extern void term_glyph ();

extern void glyph_init_render (glyph_t *g, hb_font_t *hbFont, FT_Face ftFace, gik_t *gik, hb_codepoint_t cp, hb_feature_t *feat);
extern void glyph_init_diff (glyph_t *g, glyph_t *a, glyph_t *b);
extern void glyph_term (glyph_t *g);

#endif
