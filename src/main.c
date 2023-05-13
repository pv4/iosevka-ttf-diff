#include "main.h"
#include "args.h"
#include "glyph.h"
#include "gdumper.h"
#include "font.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct { unsigned current, total; } progress_t;

static args_t args;

static hb_face_t *ohbface, *nhbface;
static hb_font_t *ohbfont, *nhbfont;

static FT_Library ft;
static FT_Face oftface, nftface;

static font_t ofont, nfont;
static hb_set_t *ocodepoints, *ovalues, *ncodepoints, *nvalues;

static void init (const char *ofile, const char *nfile) {
	hb_blob_t *b;
	
	b = hb_blob_create_from_file(ofile);
	ohbface = hb_face_create(b, 0);
	hb_blob_destroy(b);
	ohbfont = hb_font_create(ohbface);
	hb_font_set_scale(ohbfont, args.render_size, args.render_size);
	
	b = hb_blob_create_from_file(nfile);
	nhbface = hb_face_create(b, 0);
	hb_blob_destroy(b);
	nhbfont = hb_font_create(nhbface);
	hb_font_set_scale(nhbfont, args.render_size, args.render_size);
	
	FT_Init_FreeType(&ft);
	FT_New_Face(ft, ofile, 0, &oftface);
	FT_New_Face(ft, nfile, 0, &nftface);
	
	FT_Set_Pixel_Sizes(oftface, args.render_size, args.render_size);
	FT_Set_Pixel_Sizes(nftface, args.render_size, args.render_size);
	
	font_init_e(&ofont, ohbface, ohbfont);
	font_init_e(&nfont, nhbface, nhbfont);
	
	init_glyph();
	init_gdumper();
	
	ncodepoints = hb_set_create();
	nvalues = hb_set_create();
	ocodepoints = hb_set_create();
	ovalues = hb_set_create();
}

static void term () {
	hb_set_destroy(ovalues);
	hb_set_destroy(ocodepoints);
	hb_set_destroy(nvalues);
	hb_set_destroy(ncodepoints);
	
	term_gdumper();
	term_glyph();
	
	font_term(&nfont);
	font_term(&ofont);
	
	FT_Done_Face(nftface);
	FT_Done_Face(oftface);
	FT_Done_FreeType(ft);
	
	hb_font_destroy(nhbfont);
	hb_face_destroy(nhbface);
	hb_font_destroy(ohbfont);
	hb_face_destroy(ohbface);
}

static int glyph_changed (glyph_t *diff, int accuracy) {
	int ymax, xmax, y, x, by, bx, yacc, xacc;
	unsigned char *base, *p;
	
	if (!diff->height || !diff->width) return 0;
	
	yacc = diff->height < accuracy ? diff->height : accuracy;
	xacc = diff->width < accuracy ? diff->width : accuracy;
	ymax = diff->height - (yacc-1);
	xmax = diff->width - (xacc-1);
	
	base = diff->bitmap;
	for (y = 0; y != ymax; y++, base+=xacc-1)
		for (x = 0; x != xmax; x++, base++) {
			p = base;
			for (by = 0; by != yacc; by++, p+=xmax-1)
				for (bx = 0; bx != xacc; bx++, p++)
					if (!*p)
						goto next;
			
			return 1;
next:
		}
	
	return 0;
}

static void codepoint_deleted (hb_codepoint_t cp, hb_feature_t *f) {
	if (!f)
		printf("U+%04X: deleted\n", (unsigned)cp);
	else
		printf("U+%04X.%c%c%c%c=%u: deleted\n", (unsigned)cp,
			HB_UNTAG(f->tag), (unsigned)f->value);
}

static void codepoint_added (hb_codepoint_t cp, hb_feature_t *f) {
	if (!f)
		printf("U+%04X: added\n", (unsigned)cp);
	else
		printf("U+%04X.%c%c%c%c=%u: added\n", (unsigned)cp,
			HB_UNTAG(f->tag), (unsigned)f->value);
}

static void codepoint_kept (hb_codepoint_t cp, hb_feature_t *f) {
	static char out[4096];
	glyph_t o, n, d;
	int changed;
	const char *status;
	
	glyph_init_render(&o, ohbfont, oftface, cp, f);
	glyph_init_render(&n, nhbfont, nftface, cp, f);
	glyph_init_diff(&d, &o, &n);
	
	changed = glyph_changed(&d, args.accuracy);
	
	if (changed || args.log_kept) {
		status = changed ? "changed" : "kept";
		
		if (!f)
			printf("U+%04X: %s\n", (unsigned)cp, status);
		else
			printf("U+%04X.%c%c%c%c=%u: %s\n", (unsigned)cp,
				HB_UNTAG(f->tag), (unsigned)f->value,
				status);
	}
	
	if (changed && args.out_diff) {
		if (args.out_png) {
			if (!f)
				sprintf(out, "%su%04x.diff.png", args.out_png, (unsigned)cp);
			else
				sprintf(out, "%su%04x_%c%c%c%c_%u.diff.png", args.out_png, (unsigned)cp,
					HB_UNTAG(f->tag), (unsigned)f->value);
			gdumper_png_diff(&d, &o, &n, out);
		} else gdumper_ascii_diff(&d, &o, &n);
	}
	
	glyph_term(&d);
	glyph_term(&n);
	glyph_term(&o);
}

int main (int argc, char *argv[]) {
	hb_feature_t f;
	hb_codepoint_t cp;
	hb_tag_t t;
	uint32_t v;
	int ret;
	
	ret = args_init(&args, argc, argv);
	if (ret) return ret;
	
	ret = 1;
	
	init(args.old_file, args.new_file);
	
	f.start = HB_FEATURE_GLOBAL_START;
	f.end = HB_FEATURE_GLOBAL_END;
	
	for_hb_set_t (ofont.codepoints, cp)
		if (!hb_set_has(nfont.codepoints, cp))
			codepoint_deleted(cp, NULL);
	for_hb_set_t (nfont.codepoints, cp)
		if (!hb_set_has(ofont.codepoints, cp))
			codepoint_added(cp, NULL);
	for_hb_set_t (nfont.codepoints, cp)
		if (hb_set_has(ofont.codepoints, cp))
			codepoint_kept(cp, NULL);
	
	for_hb_set_t (ofont.features, t) {
		if (hb_set_has(nfont.features, t)) continue;
		
		f.tag = t;
		font_collect_feature_sets_clean(&ofont, ocodepoints, ovalues, t);
		for_hb_set_t (ovalues, v) {
			f.value = v;
			for_hb_set_t (ocodepoints, cp)
				codepoint_deleted(cp, &f);
		}
	}
	for_hb_set_t (nfont.features, t) {
		if (hb_set_has(ofont.features, t)) continue;
		
		f.tag = t;
		font_collect_feature_sets_clean(&nfont, ncodepoints, nvalues, t);
		for_hb_set_t (nvalues, v) {
			f.value = v;
			for_hb_set_t (ncodepoints, cp)
				codepoint_added(cp, &f);
		}
	}
	for_hb_set_t (nfont.features, t) {
		if (!hb_set_has(ofont.features, t)) continue;
		
		f.tag = t;
		
		font_collect_feature_sets_clean(&ofont, ocodepoints, ovalues, t);
		font_collect_feature_sets_clean(&nfont, ncodepoints, nvalues, t);
		
		for_hb_set_t (ovalues, v) {
			if (hb_set_has(nvalues, v)) continue;
			
			f.value = v;
			for_hb_set_t (ocodepoints, cp)
				codepoint_deleted(cp, &f);
		}
		for_hb_set_t (nvalues, v) {
			if (hb_set_has(ovalues, v)) continue;
			
			f.value = v;
			for_hb_set_t (ncodepoints, cp)
				codepoint_added(cp, &f);
		}
		for_hb_set_t (nvalues, v) {
			if (!hb_set_has(ovalues, v)) continue;
			
			f.value = v;
			
			for_hb_set_t (ocodepoints, cp)
				if (!hb_set_has(ncodepoints, cp))
					codepoint_deleted(cp, &f);
			for_hb_set_t (ncodepoints, cp)
				if (!hb_set_has(ocodepoints, cp))
					codepoint_added(cp, &f);
			for_hb_set_t (ncodepoints, cp)
				if (hb_set_has(ocodepoints, cp))
					codepoint_kept(cp, &f);
		}
	}
	
	ret = 0;
	
	term();
	
	return ret;
}
