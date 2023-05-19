#include "font.h"
#include "error.h"

int font_feature_is_known (const char *t) {
	return
		t[0]=='c' && t[1]=='v' ||
		t[0]=='V' && t[1]=='X' && t[2]!='L' ||
		t[0]=='N' && t[1]=='W' && t[2]=='I' && t[3]=='D' ||
		t[0]=='W' && t[1]=='W' && t[2]=='I' && t[3]=='D';
}

void font_init_e (font_t *f, hb_face_t *face, hb_font_t *font) {
	hb_tag_t fts[FONT_FEATURES_MAX];
	unsigned ftslen = sizeof fts / sizeof *fts;
	char t[5];
	unsigned i;
	hb_codepoint_t cp, gid;
	
	f->face = face;
	f->font = font;
	
	f->nominal_codepoints = hb_map_create();
	f->codepoints = hb_set_create();
	f->features = hb_set_create();
	
	hb_face_collect_unicodes(f->face, f->codepoints);
	for_hb_set_t (f->codepoints, cp) {
		hb_font_get_nominal_glyph(f->font, cp, &gid);
		if (hb_map_has(f->nominal_codepoints, gid))
			goto_throw_verbose(err, ERROR_EXIT, (
				"ERROR: glyph id %04x is nominal for multiple codepoints: u+%04x and (at least) u+%04x\n",
				gid, hb_map_get(f->nominal_codepoints, gid), cp));
		
		hb_map_set(f->nominal_codepoints, gid, cp);
	}
	
	hb_ot_layout_table_get_feature_tags(f->face, HB_OT_TAG_GSUB, 0, &ftslen, fts);
	for (i = 0; i != ftslen; i++) {
		hb_tag_to_string(fts[i], t);
		if (!font_feature_is_known(t)) continue;
		
		hb_set_add(f->features, fts[i]);
	}
	
	return;
	
catch_err:
	hb_set_destroy(f->features);
	hb_set_destroy(f->codepoints);
	hb_map_destroy(f->nominal_codepoints);
}

void font_term (font_t *f) {
	hb_set_destroy(f->features);
	hb_set_destroy(f->codepoints);
	hb_map_destroy(f->nominal_codepoints);
}

void font_collect_feature_sets_clean (font_t *f, hb_set_t *codepoints, hb_set_t *values, hb_tag_t ft) {
	hb_tag_t fts[] = { ft, HB_TAG_NONE };
	hb_codepoint_t alts[64];
	unsigned altslen;
	hb_set_t *lookups, *glyphs, *befs, *afts, *outs;
	hb_codepoint_t l, g, cp;
	unsigned i, n;
	
	lookups = hb_set_create();
	glyphs = hb_set_create();
	befs = hb_set_create();
	afts = hb_set_create();
	outs = hb_set_create();
	
	hb_ot_layout_collect_lookups(f->face, HB_OT_TAG_GSUB, NULL, NULL, fts, lookups);
	
	for_hb_set_t (lookups, l)
		hb_ot_layout_lookup_collect_glyphs(f->face, HB_OT_TAG_GSUB, l, befs, glyphs, afts, outs);
	
	hb_set_clear(codepoints);
	for_hb_set_t (glyphs, g) {
		cp = hb_map_get(f->nominal_codepoints, g);
		if (HB_MAP_VALUE_INVALID == cp) continue;
		
		hb_set_add(codepoints, cp);
	}
	
	hb_set_clear(values);
	
	if (ft == HB_TAG('W','W','I','D') || ft == HB_TAG('N','W','I','D'))
		hb_set_add(values, 1);
	else
		for_hb_set_t (glyphs, g) {
			n = 0;
			for_hb_set_t (lookups, l) {
				altslen = sizeof alts / sizeof *alts;
				hb_ot_layout_lookup_get_glyph_alternates(f->face, l, g, 0, &altslen, alts);
				for (i = 0; i != altslen; i++)
					hb_set_add(values, ++n);
			}
		}
	
	hb_set_destroy(outs);
	hb_set_destroy(afts);
	hb_set_destroy(befs);
	hb_set_destroy(glyphs);
	hb_set_destroy(lookups);
}
