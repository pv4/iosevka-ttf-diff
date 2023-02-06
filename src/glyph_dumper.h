#ifndef _20221228_055431_glyph_dumper_h
#define _20221228_055431_glyph_dumper_h

#include "glyph.h"

extern void GlyphDumper_ascii (Glyph *g);
extern void GlyphDumper_asciiDiff (Glyph *g, Glyph *a, Glyph *b);

extern void GlyphDumper_pngDiff (Glyph *g, Glyph *a, Glyph *b, const char *path);

#endif
