#ifndef h_20221228_055431_main
#define h_20221228_055431_main

#define FONT_FEATURES_MAX 512

#include <hb.h>
#include <hb-ot.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_IMAGE_H
#include FT_GLYPH_H

#define for_hb_set_t(set, i) for ((i) = HB_SET_VALUE_INVALID; hb_set_next((set), &(i)); )

#endif
