#include "main.h"
#include "args.h"
#include "glyph.h"
#include "gdumper.h"
#include "gik.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct { unsigned current, total; } progress_t;

static args_t args;

static hb_blob_t *ohbblob, *nhbblob;
static hb_face_t *ohbface, *nhbface;
static hb_font_t *ohbfont, *nhbfont;

static FT_Library ft;
static FT_Face oftface, nftface;

static hb_set_t *codepoints, *features;
static char pngpath[4096];

static void init (const char *ofile, const char *nfile) {
	ohbblob = hb_blob_create_from_file(ofile);
	ohbface = hb_face_create(ohbblob, 0);
	ohbfont = hb_font_create(ohbface);
	hb_font_set_scale(ohbfont, args.render_size, args.render_size);
	nhbblob = hb_blob_create_from_file(nfile);
	nhbface = hb_face_create(nhbblob, 0);
	nhbfont = hb_font_create(nhbface);
	hb_font_set_scale(nhbfont, args.render_size, args.render_size);
	
	FT_Init_FreeType(&ft);
	FT_New_Face(ft, ofile, 0, &oftface);
	FT_New_Face(ft, nfile, 0, &nftface);
	
	codepoints = hb_set_create();
	features = hb_set_create();
	
	FT_Set_Pixel_Sizes(oftface, args.render_size, args.render_size);
	FT_Set_Pixel_Sizes(nftface, args.render_size, args.render_size);
	
	init_glyph();
	init_gdumper();
}

static void term () {
	term_gdumper();
	term_glyph();
	
	hb_set_destroy(features);
	hb_set_destroy(codepoints);
	
	FT_Done_Face(nftface);
	FT_Done_Face(oftface);
	FT_Done_FreeType(ft);
	
	hb_font_destroy(nhbfont);
	hb_face_destroy(nhbface);
	hb_blob_destroy(nhbblob);
	hb_font_destroy(ohbfont);
	hb_face_destroy(ohbface);
	hb_blob_destroy(ohbblob);
}

static void collect_sets (hb_face_t *hbface, hb_set_t *features, hb_set_t *codepoints) {
	hb_tag_t fs[256];
	unsigned fslen = sizeof fs / sizeof *fs;
	unsigned i;
	
	// ensure the previous codepoints are kept
	hb_face_collect_unicodes(hbface, codepoints);
	
	hb_ot_layout_table_get_feature_tags(hbface, HB_OT_TAG_GSUB, 0, &fslen, fs);
	for (i = 0; i != fslen; i++) {
		int ok;
		char f[5];
		
		hb_tag_to_string(fs[i], f);
		ok =
			!strncmp(f, "cv", 2) ||
			!strncmp(f, "VX", 2) ||
			!strncmp(f, "NWID", 4) ||
			!strncmp(f, "WWID", 4);
		if (!ok) continue;
		
		hb_set_add(features, fs[i]);
	}
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

static void compare (glyph_t *o, glyph_t *n, hb_codepoint_t cp, const char *sfeat, unsigned f, float fprogress) {
	glyph_t diff;
	
	glyph_init_diff(&diff, o, n);
	
	if (args.out_png)
		if (!sfeat) sprintf(pngpath, "%su%04x.diff.png", args.out_png, cp);
		else sprintf(pngpath, "%su%04x_%s_%u.diff.png", args.out_png, cp, sfeat, f);
	
	if (glyph_changed(&diff, args.accuracy)) {
		if (!sfeat)
			printf("% 2.3f %% glyph changed: u%04x.default, %x\n", fprogress, (unsigned)cp, diff.index);
		else
			printf("% 2.3f %% glyph changed: u%04x.%s=%u, %x\n", fprogress, (unsigned)cp, sfeat, f, diff.index);
		if (args.out_diff)
			if (args.out_png) gdumper_png_diff(&diff, o, n, pngpath);
			else gdumper_ascii_diff(&diff, o, n);
	}
	else
		if (args.log_kept)
			if (!sfeat) printf("% 2.3f %% glyph kept: u%04x.default, %x\n", fprogress, (unsigned)cp, diff.index);
			else printf("% 2.3f %% glyph kept: u%04x.%s=%u, %x\n", fprogress, (unsigned)cp, sfeat, f, diff.index);
	
	glyph_term(&diff);
}

int main (int argc, char *argv[]) {
	progress_t progress;
	glyph_t oglyph, nglyph;
	gik_t ogik, ngik, ofeatgik, nfeatgik, otmpgik, ntmpgik;
	hb_feature_t feature;
	hb_codepoint_t cp, feat;
	float fprogress;
	unsigned f;
	int argserr, oeq, neq, last_value;
	char sfeat[5];
	
	argserr = args_init(&args, argc, argv);
	if (argserr) return argserr;
	
	init(args.old_file, args.new_file);
	
	collect_sets(ohbface, features, codepoints);
	collect_sets(nhbface, features, codepoints);
	
	printf("features (merged): %u\ncodepoints (merged): %u\n", hb_set_get_population(features), hb_set_get_population(codepoints));
	
	feature.start = HB_FEATURE_GLOBAL_START;
	feature.end = HB_FEATURE_GLOBAL_END;
	
	progress.total = hb_set_get_population(codepoints);
	progress.current = 0;
	
	cp = HB_SET_VALUE_INVALID;
	while (hb_set_next(codepoints, &cp)) {
		//if (cp > 0x7f) break;
		//if (cp > 0x2b) break;
//		if (cp < 0x019a) continue;
//		if (cp < 0x1d78) continue;

//		if (cp != 0x1d466) continue;
//		if (cp != 0x022f) continue;
//		if (cp != 'A') continue;
//		if (cp != 0x00f6) continue;
//		if (cp != 410 && cp != 189) continue;
//		if (cp != 0x1f147) continue;
//		if (cp != 0x2105) continue;
//		if (cp != 0x2195) continue;
//		if (cp != '*') continue;
//		if (cp != 0x1f105) continue;
//		if (cp != 0x1e62) continue;
//		if (cp != '%') continue;
		
		progress.current++;
		fprogress = 100.0 * progress.current / progress.total;
		
		glyph_init_render(&oglyph, ohbfont, oftface, &ogik, cp, NULL);
		glyph_init_render(&nglyph, nhbfont, nftface, &ngik, cp, NULL);
		
		if (!ogik.len)
			printf("% 2.3f %% glyph added: u%04x.default\n", fprogress, cp);
		if (!ngik.len)
			printf("% 2.3f %% glyph deleted: u%04x.default\n", fprogress, cp);
		if (ogik.len && ngik.len)
			compare(&oglyph, &nglyph, cp, NULL, 0, fprogress);
		
		glyph_term(&nglyph);
		glyph_term(&oglyph);
		
		feat = HB_SET_VALUE_INVALID;
		while (hb_set_next(features, &feat)) {
			feature.tag = feat;
			
			if (ogik.len) gik_copy(&ofeatgik, &ogik);
			if (ngik.len) gik_copy(&nfeatgik, &ngik);
			
			last_value = 0;
			
			for (f = 1; ; f++) {
				feature.value = f;
				
				hb_tag_to_string(feat, sfeat);
				
				glyph_init_render(&oglyph, ohbfont, oftface, &otmpgik, cp, &feature);
				glyph_init_render(&nglyph, nhbfont, nftface, &ntmpgik, cp, &feature);
				
				oeq = !ogik.len || gik_eq(&otmpgik, &ofeatgik) || gik_eq(&otmpgik, &ogik);
				neq = !ngik.len || gik_eq(&ntmpgik, &nfeatgik) || gik_eq(&ntmpgik, &ngik);
				
				if (oeq && !neq)
					printf("% 2.3f %% glyph added: u%04x.%s=%u\n", fprogress, cp, sfeat, f);
				else if (!oeq && neq)
					printf("% 2.3f %% glyph deleted: u%04x.%s=%u\n", fprogress, cp, sfeat, f);
				else if (!oeq && !neq)
					compare(&oglyph, &nglyph, cp, sfeat, f, fprogress);
				else last_value = 1;
				
				glyph_term(&nglyph);
				glyph_term(&oglyph);
				
				if (last_value)
					break;
				
				if (!oeq) gik_copy(&ofeatgik, &otmpgik);
				if (!neq) gik_copy(&nfeatgik, &ntmpgik);
			}
		}
	}
	
	term();
	
	return 0;
}
