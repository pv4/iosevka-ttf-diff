#include "compare_main.h"
#include "map_main.h"

#include "error.h"
#include "lib.h"

#include <stdio.h>
#include <string.h>

static void usage (FILE *f, char *argv[]) {
	const char *bin = basename(argv[0]);
	
	fprintf(f,
"Usage:\n"
" %s <command> [<arguments>]\n"
" %s <command> -?\n"
"\n"
"Commands:\n"
" compare        Compare two release ttf files of Iosevka and log/output the\n"
"                differencies (primary command)\n"
" map            Map old codepoint variant codes to new ones\n"
"\n"
"iosevka-ttf-diff version %s\n",
	bin, bin, VERSION);
}

enum { CMD_COMPARE, CMD_MAP };
static int cmd;

static int args (int argc, char *argv[]) {
	if (1 == argc) {
		usage(stdout, argv);
		return 1;
	}
	else if (!strcmp("compare", argv[1])) cmd = CMD_COMPARE;
	else if (!strcmp("map", argv[1])) cmd = CMD_MAP;
	else
		goto_throw_verbose(err, ERROR_USAGE,
			("%s: unrecognized command: '%s'\n", argv[0], argv[1]));
	
	return 0;
	
catch_err:
	usage(stderr, argv);
	return 0;
}

int main (int argc, char *argv[]) {
	int status;
	
	status = 2;
	
	if (args(argc, argv)) return 0;
	goto_if_rethrow(args);
	
	switch (cmd) {
	case CMD_COMPARE: status = compare_main(argc, argv); break;
	case CMD_MAP: status = map_main(argc, argv); break;
	}
	
catch_args:
	return status;
}
