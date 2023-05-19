#include "map_main.h"

#include "lib.h"
#include "error.h"
#include "main.h"
#include "font.h"
#include "vmap.h"

#include <stdio.h>
#include <unistd.h>

static const char *vmap_file;
static vmap_t vmap;

static void usage (FILE *f, char *argv[]) {
	fprintf(f,
"Usage:\n"
" %s %s [<options>]\n"
"\n"
"Map old codepoint variant codes to new ones\n"
"\n"
"Options:\n"
" -m <file>      use the variant map file for proper mapping in case\n"
"                tags/ranks of some codepoint variants changed between\n"
"                releases\n"
" -?             display this help and exit\n"
"\n"
"Examples:\n"
"1. Map a single variant and return the mapping. The below command outputs\n"
"a single line containing \"cv90=2:cv96=2\" (without quotes).\n"
" echo \"cv90=2\" |./iosevka-ttf-diff map -m ../vmaps/22.0.0.vmap\n"
"\n"
"iosevka-ttf-diff version %s\n",
	basename(argv[0]), argv[1], VERSION);
}

static int args (int argc, char *argv[]) {
	vmap_file = NULL;
	
	optind = 2;
	
	for (;;)
		switch (getopt(argc, argv, ":m:")) {
		case 'm':
			vmap_file = optarg;
			if (-1 == access(vmap_file, R_OK))
				goto_throw_verbose(err, ERROR_USAGE,
					("%s %s: unreadable variant map: '%s'\n", argv[0], argv[1], vmap_file));
			break;
		case '?':
			if ('?' != optopt)
				goto_throw_verbose(err, ERROR_USAGE,
					("%s %s: unrecognized option: '-%c'\n", argv[0], argv[1], optopt));
			usage(stdout, argv);
			return 1;
		case ':':
			goto_throw_verbose(err, ERROR_USAGE,
				("%s %s: option requires an argument: '-%c'\n", argv[0], argv[1], optopt));
		case -1:
			return 0;
		}
	
catch_err:
	usage(stderr, argv);
	return 0;
}

static int buf_parse (char *buf, hb_codepoint_t *t, uint32_t *v) {
	int i, n;
	
	if (!(
		font_feature_is_known(buf) &&
		'=' == buf[4] &&
		isdigit_c(buf[5]))) return 0;
	
	n = 0;
	for (i = 5; buf[i] && isdigit_c(buf[i]); i++)
		n = 10*n + buf[i]-'0';
	if (buf[i]) return 0;
	
	*t = HB_TAG(buf[0], buf[1], buf[2], buf[3]);
	*v = n;
	return 1;
}

int map_main (int argc, char *argv[]) {
	// ????=??\n\0
	char buf[9];
	size_t buflen;
	hb_codepoint_t ot, nt;
	uint32_t ov, nv;
	int status;
	
	status = 2;
	
	if (args(argc, argv)) return 0;
	goto_if_rethrow(args);
	
	status = 1;
	
	vmap_init(&vmap);
	if (vmap_file)
		vmap_load_e(&vmap, vmap_file);
	
	while (fgets(buf, sizeof buf/sizeof *buf, stdin)) {
		buflen = strlen(buf);
		if ('\n' == buf[buflen-1]) buf[buflen-1] = '\0';
		
		if (!buf_parse(buf, &ot, &ov))
			goto_throw_verbose(parse, ERROR_EXIT,
				("%s %s: invalid variant map: '%s'", argv[0], argv[1], buf));
		
		vmap_upgrade_value(&vmap, &nt, &nv, ot, ov);
		
		if (HB_TAG_NONE == nt)
			printf("%c%c%c%c=%u:=\n",
				HB_UNTAG(ot), (unsigned)ov);
		else
			printf("%c%c%c%c=%u:%c%c%c%c=%u\n",
				HB_UNTAG(ot), (unsigned)ov,
				HB_UNTAG(nt), (unsigned)nv);
	}
	
	status = 0;
	
catch_parse:
	vmap_term(&vmap);
	
catch_args:
	return status;
}
