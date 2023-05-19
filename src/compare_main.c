#include "compare_main.h"

#include "error.h"
#include "compare_args.h"
#include "main.h"
#include "glyph.h"
#include "gdumper.h"
#include "vmap.h"
#include "font.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct {
	unsigned
		progress, progress_limit,
		ocodepoints, ofeatures,
		ncodepoints, nfeatures,
		deleted, added, changed, kept;
} stat_t;

static compare_args_t args;

static hb_face_t *ohbface, *nhbface;
static hb_font_t *ohbfont, *nhbfont;

static FT_Library ft;
static FT_Face oftface, nftface;

static stat_t stat;
static font_t ofont, nfont;
static vmap_t vmap;
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
	
	vmap_init(&vmap);
	
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
	
	vmap_term(&vmap);
	
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

static inline float progress ()
	{ return 100.0*stat.progress/stat.progress_limit; }

static void codepoint_deleted (hb_codepoint_t cp, hb_feature_t *of) {
	stat.deleted++;
	
	if (args.log & COMPARE_ARGS_LOG_DELETED)
		if (!of)
			printf("%6.3f %% U+%04X: deleted\n",
				progress(), (unsigned)cp);
		else
			printf("%6.3f %% U+%04X.%c%c%c%c=%u: deleted\n",
				progress(), (unsigned)cp,
				HB_UNTAG(of->tag), (unsigned)of->value);
}

static void codepoint_added (hb_codepoint_t cp, hb_feature_t *nf) {
	stat.added++;
	
	if (args.log & COMPARE_ARGS_LOG_ADDED)
		if (!nf)
			printf("%6.3f %% U+%04X: added\n",
				progress(), (unsigned)cp);
		else
			printf("%6.3f %% U+%04X.%c%c%c%c=%u: added\n",
				progress(), (unsigned)cp,
				HB_UNTAG(nf->tag), (unsigned)nf->value);
}

static void codepoint_kept (hb_codepoint_t cp, hb_feature_t *of, hb_feature_t *nf) {
	static char out[4096];
	glyph_t o, n, d;
	int changed;
	const char *status;
	
	glyph_init_render(&o, ohbfont, oftface, cp, of);
	glyph_init_render(&n, nhbfont, nftface, cp, nf);
	glyph_init_diff(&d, &o, &n);
	
	changed = glyph_changed(&d, args.accuracy);
	if (changed) stat.changed++;
	else stat.kept++;
	
	if (
		changed && args.log & COMPARE_ARGS_LOG_CHANGED ||
		!changed && args.log & COMPARE_ARGS_LOG_KEPT) {
		
		status = changed ? "changed" : "kept";
		
		if (!nf)
			printf("%6.3f %% U+%04X: %s\n",
				progress(), (unsigned)cp,
				status);
		else if (of->tag == nf->tag && of->value == nf->value)
			printf("%6.3f %% U+%04X.%c%c%c%c=%u: %s\n",
				progress(), (unsigned)cp,
				HB_UNTAG(nf->tag), (unsigned)nf->value,
				status);
		else
			printf("%6.3f %% U+%04X.%c%c%c%c=%u (was %c%c%c%c=%u): %s\n",
				progress(), (unsigned)cp,
				HB_UNTAG(nf->tag), (unsigned)nf->value,
				HB_UNTAG(of->tag), (unsigned)of->value,
				status);
	}
	
	if (changed && args.out & COMPARE_ARGS_OUT_CHANGED) {
		if (args.out_png) {
			if (!nf)
				sprintf(out, "%su%04x.diff.png", args.out_png, (unsigned)cp);
			else
				sprintf(out, "%su%04x_%c%c%c%c_%u.diff.png", args.out_png, (unsigned)cp,
					HB_UNTAG(nf->tag), (unsigned)nf->value);
			gdumper_png_diff(&d, &o, &n, out);
		} else gdumper_ascii_diff(&d, &o, &n);
	}
	
	glyph_term(&d);
	glyph_term(&n);
	glyph_term(&o);
}

int compare_main (int argc, char *argv[]) {
	hb_feature_t of, nf;
	hb_codepoint_t cp;
	hb_tag_t t, ot, nt;
	uint32_t ov, nv;
	int status;
	
	status = 2;
	
	if (compare_args_init(&args, argc, argv)) return 0;
	goto_if_rethrow(args);
	
	status = 1;
	
	init(args.old_file, args.new_file);
	
	if (args.vmap_file)
		vmap_load_e(&vmap, args.vmap_file);
	
	stat.ocodepoints = hb_set_get_population(ofont.codepoints);
	stat.ofeatures = hb_set_get_population(ofont.features);
	stat.ncodepoints = hb_set_get_population(nfont.codepoints);
	stat.nfeatures = hb_set_get_population(nfont.features);
	
	stat.deleted = stat.added = stat.changed = stat.kept = 0;
	
	of.start = nf.start = HB_FEATURE_GLOBAL_START;
	of.end = nf.end = HB_FEATURE_GLOBAL_END;
	
	stat.progress_limit =
		hb_set_get_population(ofont.codepoints) +
		hb_set_get_population(nfont.codepoints);
	for_hb_set_t (ofont.features, ot) {
		font_collect_feature_sets_clean(&ofont, ocodepoints, ovalues, ot);
		stat.progress_limit +=
			hb_set_get_population(ocodepoints) * hb_set_get_population(ovalues);
	}
	for_hb_set_t (nfont.features, nt) {
		font_collect_feature_sets_clean(&nfont, ncodepoints, nvalues, nt);
		stat.progress_limit +=
			hb_set_get_population(ncodepoints) * hb_set_get_population(nvalues);
	}
	stat.progress = 0;
	
	for_hb_set_t (ofont.codepoints, cp)
		if (!hb_set_has(nfont.codepoints, cp))
			codepoint_deleted(cp, NULL), stat.progress++;
	for_hb_set_t (nfont.codepoints, cp)
		if (!hb_set_has(ofont.codepoints, cp))
			codepoint_added(cp, NULL), stat.progress++;
	for_hb_set_t (nfont.codepoints, cp)
		if (hb_set_has(ofont.codepoints, cp))
			codepoint_kept(cp, NULL, NULL), stat.progress+=2;
	
	for_hb_set_t (ofont.features, ot) {
		vmap_upgrade_tag(&vmap, &nt, ot);
		if (nt != HB_TAG_NONE && hb_set_has(nfont.features, nt)) continue;
		
		of.tag = ot;
		font_collect_feature_sets_clean(&ofont, ocodepoints, ovalues, ot);
		for_hb_set_t (ovalues, ov) {
			of.value = ov;
			for_hb_set_t (ocodepoints, cp)
				codepoint_deleted(cp, &of), stat.progress++;
		}
	}
	for_hb_set_t (nfont.features, nt) {
		vmap_downgrade_tag(&vmap, &ot, nt);
		if (ot != HB_TAG_NONE && hb_set_has(ofont.features, ot)) continue;
		
		nf.tag = nt;
		font_collect_feature_sets_clean(&nfont, ncodepoints, nvalues, nt);
		for_hb_set_t (nvalues, nv) {
			nf.value = nv;
			for_hb_set_t (ncodepoints, cp)
				codepoint_added(cp, &nf), stat.progress++;
		}
	}
	for_hb_set_t (nfont.features, nt) {
		vmap_downgrade_tag(&vmap, &ot, nt);
		if (ot == HB_TAG_NONE || !hb_set_has(ofont.features, ot)) continue;
		
		of.tag = ot; nf.tag = nt;
		
		font_collect_feature_sets_clean(&ofont, ocodepoints, ovalues, ot);
		font_collect_feature_sets_clean(&nfont, ncodepoints, nvalues, nt);
		
		for_hb_set_t (ovalues, ov) {
			vmap_upgrade_value(&vmap, &t, &nv, ot, ov);
			if (t != HB_TAG_NONE && hb_set_has(nvalues, nv)) continue;
			
			of.value = ov;
			for_hb_set_t (ocodepoints, cp)
				codepoint_deleted(cp, &of), stat.progress++;
		}
		for_hb_set_t (nvalues, nv) {
			vmap_downgrade_value(&vmap, &t, &ov, nt, nv);
			if (t != HB_TAG_NONE && hb_set_has(ovalues, ov)) continue;
			
			nf.value = nv;
			for_hb_set_t (ncodepoints, cp)
				codepoint_added(cp, &nf), stat.progress++;
		}
		for_hb_set_t (nvalues, nv) {
			vmap_downgrade_value(&vmap, &t, &ov, nt, nv);
			if (t == HB_TAG_NONE || !hb_set_has(ovalues, ov)) continue;
			
			of.value = ov; nf.value = nv;
			
			for_hb_set_t (ocodepoints, cp)
				if (!hb_set_has(ncodepoints, cp))
					codepoint_deleted(cp, &of), stat.progress++;
			for_hb_set_t (ncodepoints, cp)
				if (!hb_set_has(ocodepoints, cp))
					codepoint_added(cp, &nf), stat.progress++;
			for_hb_set_t (ncodepoints, cp)
				if (hb_set_has(ocodepoints, cp))
					codepoint_kept(cp, &of, &nf), stat.progress+=2;
		}
	}
	
	printf(
		"Codepoints: %u old/%u new, variant features: %u old/%u new\n"
			"Codepoint variants: %u deleted, %u added, %u changed, %u kept\n",
		stat.ocodepoints, stat.ncodepoints, stat.ofeatures, stat.nfeatures,
		stat.deleted, stat.added, stat.changed, stat.kept);
	
	status = 0;
	
	term();
	
catch_args:
	return status;
}
