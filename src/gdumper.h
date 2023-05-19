/* src/gdumper.h */

#ifndef h_20221228_055431_gdumper
#define h_20221228_055431_gdumper

#include "glyph.h"

extern void init_gdumper ();
extern void term_gdumper ();

extern void gdumper_ascii (glyph_t *g);
extern void gdumper_ascii_diff (glyph_t *g, glyph_t *a, glyph_t *b);
extern void gdumper_png_diff (glyph_t *g, glyph_t *a, glyph_t *b, const char *path);

#endif
