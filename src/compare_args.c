/* src/compare_args.c */

#include "compare_args.h"

#include "error.h"
#include "lib.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void usage (FILE *f, char *argv[]) {
	fprintf(f,
"Usage:\n"
" %s %s [<options>] <old-ttf> <new-ttf>\n"
"\n"
"Compare two release ttf files of Iosevka and log/output the differencies.\n"
"\n"
"Options:\n"
" -m <file>              use the variant map file to proper detect shape\n"
"                        differences in case tags/ranks of some codepoint\n"
"                        variants changed between <old-ttf> and <new-ttf>\n"
" -s <number>            render size for codepoint variant glyphs (default is\n"
"                        512)\n"
" -a <number>            accuracy to only consider codepoint variant shapes\n"
"                        different if the shapes have at least one continous\n"
"                        square area of the given size where no pixels exist\n"
"                        that are equal between the shapes (default is 5); 1\n"
"                        means exact shape match\n"
" -l<filter>             codepoint variants to log (if none are given\n"
"                        '-ld -la -lc' are assumed)\n"
"    -ld                 only present in <old-ttf>\n"
"    -la                 only present in <new-ttf>\n"
"    -lc                 having different shapes in <old-ttf> and <new-ttf> \n"
"    -lk                 having similar shapes in <old-ttf> and <new-ttf> \n"
"    -lA                 all of the above\n"
"    -lN                 don't log anything\n"
" -o<filter>             information type to output (if none are given '-oc' is\n"
"                        assumed)\n"
"    -od                 images of codepoint variants only present in <old-ttf>\n"
"    -oa                 images of codepoint variants only present in <new-ttf>\n"
"    -oc                 images of codepoint variants with different shapes in\n"
"                        <old-ttf> and <new-ttf>, that contain shapes of both\n"
"                        variants and a shape visually demonstating the\n"
"                        difference between the variants; only codepoint\n"
"                        variants present both in <old-ttf> and in <new-ttf>\n"
"                        are considered\n"
"    -oA                 all of the above\n"
"    -oN                 don't output anything\n"
" -f <output-format>     output format (by default outputs to a standard output\n"
"                        in a format something like ascii art)\n"
"    png:<prefix>        output to png files with names starting with <prefix>\n"
" -?                     display this help and exit\n"
"\n"
"Examples:\n"
"1. Just log all the changes without creating output images. Use reduced (less\n"
"accurate) render size and the best possible accuracy (pixel-by-pixel match of\n"
"the rendered codepoint variant shapes).\n"
" iosevka-ttf-diff compare \\\n"
"  -lA -oN -s 128 -a 1 \\\n"
"  ./iosevka21.0.0-heavy.ttf ./iosevka22.0.0-heavy.ttf\n"
"\n"
"2. Only log codepoint variants changed between 18.0.0 and 21.0.0 releases.\n"
"Output diffs to png files with names like ./out-u0179_cv25_9.diff.png (i.e.\n"
"to the current directory).\n"
" iosevka-ttf-diff compare \\\n"
"  -lc -f png:./out- \\\n"
"  ./iosevka18.0.0-heavy.ttf ./iosevka21.0.0-heavy.ttf\n"
"\n"
"3. Log deleted/added/changed codepoint variants, use the given variant map\n"
"file, output diffs to png files with names like\n"
"./iosevka22.0.0-heavy/u0179_cv25_9.diff.png (i.e. to ./iosevka22.0.0-heavy\n"
"directory).\n"
" iosevka-ttf-diff compare \\\n"
"  -m ../vmaps/22.0.0.vmap \\\n"
"  -f png:./iosevka22.0.0-heavy/ \\\n"
"  ./iosevka21.0.0-heavy.ttf ./iosevka22.0.0-heavy.ttf\n"
"\n"
"iosevka-ttf-diff version %s\n",
	basename(argv[0]), argv[1], VERSION);
}

static unsigned n_parse (const char *p) {
	int i;
	unsigned n;
	
	if (p[0] == '0') return 0;
	
	n = 0;
	for (i = 0; p[i]; i++) {
		if (!isdigit_c(p[i])) return 0;
		n = n*10 + p[i]-'0';
	}
	
	return n;
}

int compare_args_init (compare_args_t *a, int argc, char *argv[]) {
	int i, log_reset, out_reset;
	
	a->render_size = 512;
	a->accuracy = 5;
	a->log = COMPARE_ARGS_LOG_DELETED|COMPARE_ARGS_LOG_ADDED|COMPARE_ARGS_LOG_CHANGED;
	a->out = COMPARE_ARGS_OUT_CHANGED;
	a->vmap_file = a->out_png = a->old_file = a->new_file = NULL;
	
	log_reset = out_reset = 1;
	optind = 2;
	
	for (;;)
		switch (getopt(argc, argv, ":m:s:a:l:o:f:")) {
		case 'm':
			a->vmap_file = optarg;
			if (-1 == access(a->vmap_file, R_OK))
				goto_throw_verbose(err, ERROR_USAGE,
					("%s: unreadable variant map: '%s'\n", argv[0], a->vmap_file));
			break;
		case 's':
			a->render_size = n_parse(optarg);
			if (!a->render_size)
				goto_throw_verbose(err, ERROR_USAGE,
					("%s: unrecognized render size: '%s'\n", argv[0], optarg));
			break;
		case 'a':
			a->accuracy = n_parse(optarg);
			if (!a->accuracy)
				goto_throw_verbose(err, ERROR_USAGE,
					("%s: unrecognized accuracy: '%s'\n", argv[0], optarg));
			break;
		case 'l':
			if (log_reset) a->log = 0, log_reset = 0;
			for (i = 0; optarg[i]; i++)
				switch(optarg[i]) {
				case 'd': a->log |= COMPARE_ARGS_LOG_DELETED; break;
				case 'a': a->log |= COMPARE_ARGS_LOG_ADDED; break;
				case 'c': a->log |= COMPARE_ARGS_LOG_CHANGED; break;
				case 'k': a->log |= COMPARE_ARGS_LOG_KEPT; break;
				case 'A':
					a->log =
						COMPARE_ARGS_LOG_DELETED |
						COMPARE_ARGS_LOG_ADDED |
						COMPARE_ARGS_LOG_CHANGED |
						COMPARE_ARGS_LOG_KEPT;
					break;
				case 'N': a->log = 0; break;
				default:
					goto_throw_verbose(err, ERROR_USAGE,
						("%s: unrecognized log filter: '%c'\n", argv[0], optarg[i]));
				}
			break;
		case 'o':
			if (out_reset) a->out = 0, out_reset = 0;
			for (i = 0; optarg[i]; i++)
				switch(optarg[i]) {
				case 'd': a->out |= COMPARE_ARGS_OUT_DELETED; break;
				case 'a': a->out |= COMPARE_ARGS_OUT_ADDED; break;
				case 'c': a->out |= COMPARE_ARGS_OUT_CHANGED; break;
				case 'A':
					a->out =
						COMPARE_ARGS_OUT_DELETED |
						COMPARE_ARGS_OUT_ADDED |
						COMPARE_ARGS_OUT_CHANGED;
					break;
				case 'N': a->out = 0; break;
				default:
					goto_throw_verbose(err, ERROR_USAGE,
						("%s: unrecognized output filter: '%c'\n", argv[0], optarg[i]));
				}
			break;
		case 'f':
			if (strncmp(optarg, "png:", 4))
					goto_throw_verbose(err, ERROR_USAGE,
						("%s: unrecognized format prefix: '%s'\n", argv[0], optarg));
			a->out_png = optarg+4;
			break;
		case '?':
			if ('?' != optopt)
				goto_throw_verbose(err, ERROR_USAGE,
					("%s: unrecognized option: '-%c'\n", argv[0], optopt));
			usage(stdout, argv);
			return 1;
		case ':':
			goto_throw_verbose(err, ERROR_USAGE,
				("%s: option requires an argument: '-%c'\n", argv[0], optopt));
		case -1:
			if (optind != argc-2)
				goto_throw_verbose(err, ERROR_USAGE,
					("%s: missing %s ttf\n", argv[0], optind+1 == argc ? "new" : "old"));
			
			a->old_file = argv[optind+0];
			a->new_file = argv[optind+1];
			
			if (-1 == access(a->old_file, R_OK))
				goto_throw_verbose(err, ERROR_USAGE,
					("%s: unreadable old ttf: '%s'\n", argv[0], a->old_file));
			
			if (-1 == access(a->new_file, R_OK))
				goto_throw_verbose(err, ERROR_USAGE,
					("%s: unreadable new ttf: '%s'\n", argv[0], a->new_file));
			
			return 0;
		}
	
catch_err:
	usage(stderr, argv);
	return 0;
}
