#ifndef _20221228_055431_glyph_dumper_h
#define _20221228_055431_glyph_dumper_h

#include "glyph.h"

extern void glyph_dumper_ascii (glyph_t *g);
extern void glyph_dumper_ascii_diff (glyph_t *g, glyph_t *a, glyph_t *b);

extern void glyph_dumper_png_diff (glyph_t *g, glyph_t *a, glyph_t *b, const char *path);

#endif
