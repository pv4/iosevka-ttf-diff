#include "main.h"
#include "glyph.h"
#include "glyph_dumper.h"
#include "gik.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct { unsigned current, total; } Progress;

static hb_blob_t *oHbBlob, *nHbBlob;
static hb_face_t *oHbFace, *nHbFace;
static hb_font_t *oHbFont, *nHbFont;

static FT_Library ft;
static FT_Face oFtFace, nFtFace;

static hb_set_t *codepoints, *features;

static int renderSize, accuracy, logKept, outputDiff, outputNew, outputOld;
static const char *outputPng;
static const char *ofile, *nfile;

static char pngpath[4096];

static const char *argparam (const char *arg, const char *opt) {
	unsigned len;
	
	len = strlen(opt);
	return strncmp(opt, arg, len) ? NULL : arg+len;
}

static int args (int argc, char *argv[]) {
	FILE *f;
	const char *p;
	unsigned i;
	int err, file;
	
	renderSize = 512;
	logKept = 0;
	accuracy = 5;
	outputDiff = 1;
	outputNew = outputOld = 0;
	outputPng = NULL;
	ofile = nfile = NULL;
	
	file = 'o';
	err = 0;
	for (i = 1; i != argc && !err; i++)
		if ('-' != argv[i][0])
			switch (file) {
			case 'o':
			case 'n':
				f = fopen(argv[i], "r");
				if (!f)
					{ err = 1; break; }
				fclose(f);
				
				switch (file) {
				case 'o': ofile = argv[i]; file = 'n'; break;
				case 'n': nfile = argv[i]; file = '-'; break;
				}
				break;
			default:
				err = 1;
				break;
			}
		else if (p = argparam(argv[i], "-renderSize="))
			renderSize = atoi(p);
		else if (p = argparam(argv[i], "-outputTarget=")) {
			if (!strncmp(p, "png:", 4)) outputPng = p+4;
			else err = 1;
		}
		else if (p = argparam(argv[i], "-outputSelect=")) {
			outputDiff = outputNew = outputOld = 0;
			while (*p)
				if (!strncmp(p, "diff,", 5)) { p += 5; outputDiff = 1; }
				else if (!strcmp(p, "diff")) { p += 4; outputDiff = 1; }
				else if (!strncmp(p, "new,", 4)) { p += 4; outputNew = 1; }
				else if (!strcmp(p, "new")) { p += 3; outputNew = 1; }
				else if (!strncmp(p, "old,", 4)) { p += 4; outputOld = 1; }
				else if (!strcmp(p, "old")) { p += 3; outputOld = 1; }
				else { err = 1; break; }
		}
		else if (p = argparam(argv[i], "-logKept"))
			logKept = 1;
		else if (p = argparam(argv[i], "-accuracy="))
			accuracy = atoi(p);
		else err = 1;
	if (!ofile || !nfile || accuracy < 1) err = 1;
	
	if (!err) return 0;
	
	fprintf(stderr,
		"Usage: \n"
		"  %s [OPTION]... /path/to/old.ttf /path/to/new.ttf\n"
		"\n"
		"Options:\n"
		"  -renderSize=512\n"
		"             Size of rendered glyphs.\n"
		"  -outputTarget=png:/output/pngs/prefix\n"
		"             Output each change to png file instead of stdout. File\n"
		"             name is appended to the value provided,\n"
		"             e.g. /output/pngs/prefixu002e.diff.png . If providing\n"
		"             a directory end the arument with a slash,\n"
		"             e.g. -outputTarget=png:/path/to/pngs/dir/ .\n"
		"  -outputSelect=diff\n"
		"             Changes to output, separated by comma. Possible values\n"
		"             are: diff, new, old .\n"
		"  -accuracy=5\n"
		"             Consider glyphs different only if at least one continous\n"
		"             area exists of the given size (5px x 5px) or larger\n"
		"             which is ПОЛНОСТЬЮ different in both glyphs\n"
		"  -logKept   Log glyphs that are not changed.\n"
		"  -?, -h, -help, --help\n"
		"             Show this message.\n"
		"\n"
		"Version: %s\n",
		argv[0], VERSION);
	return 1;
}

static void init (const char *ofile, const char *nfile) {
	oHbBlob = hb_blob_create_from_file(ofile);
	oHbFace = hb_face_create(oHbBlob, 0);
	oHbFont = hb_font_create(oHbFace);
	hb_font_set_scale(oHbFont, renderSize, renderSize);
	nHbBlob = hb_blob_create_from_file(nfile);
	nHbFace = hb_face_create(nHbBlob, 0);
	nHbFont = hb_font_create(nHbFace);
	hb_font_set_scale(nHbFont, renderSize, renderSize);
	
	FT_Init_FreeType(&ft);
	FT_New_Face(ft, ofile, 0, &oFtFace);
	FT_New_Face(ft, nfile, 0, &nFtFace);
	
	codepoints = hb_set_create();
	features = hb_set_create();
	
	FT_Set_Pixel_Sizes(oFtFace, renderSize, renderSize);
	FT_Set_Pixel_Sizes(nFtFace, renderSize, renderSize);
	
	Glyph_init();
}

static void term () {
	Glyph_term();
	
	hb_set_destroy(features);
	hb_set_destroy(codepoints);
	
	FT_Done_Face(nFtFace);
	FT_Done_Face(oFtFace);
	FT_Done_FreeType(ft);
	
	hb_font_destroy(nHbFont);
	hb_face_destroy(nHbFace);
	hb_blob_destroy(nHbBlob);
	hb_font_destroy(oHbFont);
	hb_face_destroy(oHbFace);
	hb_blob_destroy(oHbBlob);
}

static void collectSets (hb_face_t *hbFace, hb_set_t *features, hb_set_t *codepoints) {
	hb_tag_t fs[256];
	unsigned fslen = sizeof fs / sizeof *fs;
	unsigned i;
	
	// ensure the previous codepoints are kept
	hb_face_collect_unicodes(hbFace, codepoints);
	
	hb_ot_layout_table_get_feature_tags(hbFace, HB_OT_TAG_GSUB, 0, &fslen, fs);
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

static int glyphChanged (Glyph *diff, int accuracy) {
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

static void compare (Glyph *o, Glyph *n, hb_codepoint_t cp, const char *sfeat, unsigned f, float fprogress) {
	Glyph diff;
	
	glyph_initDiff(&diff, o, n);
	
	if (outputPng)
		if (!sfeat) sprintf(pngpath, "%su%04x.diff.png", outputPng, cp);
		else sprintf(pngpath, "%su%04x_%s_%u.diff.png", outputPng, cp, sfeat, f);
	
	if (glyphChanged(&diff, accuracy)) {
		if (!sfeat)
			printf("% 2.3f %% glyph changed: u%04x.default, %x\n", fprogress, (unsigned)cp, diff.index);
		else
			printf("% 2.3f %% glyph changed: u%04x.%s=%u, %x\n", fprogress, (unsigned)cp, sfeat, f, diff.index);
		if (outputDiff)
			if (outputPng) GlyphDumper_pngDiff(&diff, o, n, pngpath);
			else GlyphDumper_asciiDiff(&diff, o, n);
	}
	else
		if (logKept)
			if (!sfeat) printf("% 2.3f %% glyph kept: u%04x.default, %x\n", fprogress, (unsigned)cp, diff.index);
			else printf("% 2.3f %% glyph kept: u%04x.%s=%u, %x\n", fprogress, (unsigned)cp, sfeat, f, diff.index);
	
	glyph_term(&diff);
}

int main (int argc, char *argv[]) {
	Progress progress;
	Glyph oglyph, nglyph;
	Gik ogik, ngik, ofeatgik, nfeatgik, otmpgik, ntmpgik;
	hb_feature_t feature;
	hb_codepoint_t cp, feat;
	float fprogress;
	unsigned f;
	int argserr, oeq, neq, lastValue;
	char sfeat[5];
	
	argserr = args(argc, argv);
	if (argserr) return argserr;
	
	init(ofile, nfile);
	
	collectSets(oHbFace, features, codepoints);
	collectSets(nHbFace, features, codepoints);
	
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
		
		glyph_initRender(&oglyph, oHbFont, oFtFace, &ogik, cp, NULL);
		glyph_initRender(&nglyph, nHbFont, nFtFace, &ngik, cp, NULL);
		
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
			
			lastValue = 0;
			
			for (f = 1; ; f++) {
				feature.value = f;
				
				hb_tag_to_string(feat, sfeat);
				
				glyph_initRender(&oglyph, oHbFont, oFtFace, &otmpgik, cp, &feature);
				glyph_initRender(&nglyph, nHbFont, nFtFace, &ntmpgik, cp, &feature);
				
				oeq = !ogik.len || gik_eq(&otmpgik, &ofeatgik) || gik_eq(&otmpgik, &ogik);
				neq = !ngik.len || gik_eq(&ntmpgik, &nfeatgik) || gik_eq(&ntmpgik, &ngik);
				
				if (oeq && !neq)
					printf("% 2.3f %% glyph added: u%04x.%s=%u\n", fprogress, cp, sfeat, f);
				else if (!oeq && neq)
					printf("% 2.3f %% glyph deleted: u%04x.%s=%u\n", fprogress, cp, sfeat, f);
				else if (!oeq && !neq)
					compare(&oglyph, &nglyph, cp, sfeat, f, fprogress);
				else lastValue = 1;
				
				glyph_term(&nglyph);
				glyph_term(&oglyph);
				
				if (lastValue)
					break;
				
				if (!oeq) gik_copy(&ofeatgik, &otmpgik);
				if (!neq) gik_copy(&nfeatgik, &ntmpgik);
			}
		}
	}
	
	term();
	
	return 0;
}
