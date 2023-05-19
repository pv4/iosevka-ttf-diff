#ifndef h_20221228_055431_font
#define h_20221228_055431_font

#include "main.h"

typedef struct {
	hb_font_t *font;
	hb_face_t *face;
	hb_map_t *nominal_codepoints;
	hb_set_t *codepoints, *features;
} font_t;

extern int font_feature_is_known (const char *t);
extern void font_init_e (font_t *f, hb_face_t *face, hb_font_t *font);
extern void font_term (font_t *f);
extern void font_collect_feature_sets_clean (font_t *f, hb_set_t *codepoints, hb_set_t *values, hb_tag_t ft);

#endif
