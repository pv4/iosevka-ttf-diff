#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *argparam (const char *arg, const char *opt) {
	unsigned len;
	
	len = strlen(opt);
	return strncmp(opt, arg, len) ? NULL : arg+len;
}

int args_init (args_t *a, int argc, char *argv[]) {
	FILE *f;
	const char *p;
	unsigned i;
	int err, file;
	
	a->render_size = 512;
	a->accuracy = 5;
	a->log_kept = 0;
	a->out_diff = 1;
	a->out_new = a->out_old = 0;
	a->out_png = NULL;
	a->old_file = a->new_file = NULL;
	
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
				case 'o': a->old_file = argv[i]; file = 'n'; break;
				case 'n': a->new_file = argv[i]; file = '-'; break;
				}
				break;
			default:
				err = 1;
				break;
			}
		else if (p = argparam(argv[i], "-renderSize="))
			a->render_size = atoi(p);
		else if (p = argparam(argv[i], "-accuracy="))
			a->accuracy = atoi(p);
		else if (p = argparam(argv[i], "-logKept"))
			a->log_kept = 1;
		else if (p = argparam(argv[i], "-outputTarget=")) {
			if (!strncmp(p, "png:", 4)) a->out_png = p+4;
			else err = 1;
		}
		else if (p = argparam(argv[i], "-outputSelect=")) {
			a->out_diff = a->out_new = a->out_old = 0;
			while (*p)
				if (!strncmp(p, "diff,", 5)) { p += 5; a->out_diff = 1; }
				else if (!strcmp(p, "diff")) { p += 4; a->out_diff = 1; }
				else if (!strncmp(p, "new,", 4)) { p += 4; a->out_new = 1; }
				else if (!strcmp(p, "new")) { p += 3; a->out_new = 1; }
				else if (!strncmp(p, "old,", 4)) { p += 4; a->out_old = 1; }
				else if (!strcmp(p, "old")) { p += 3; a->out_old = 1; }
				else { err = 1; break; }
		}
		else err = 1;
	if (!a->old_file || !a->new_file || a->accuracy < 1) err = 1;
	
	if (!err) return 0;
	
	fprintf(stderr,
		"Usage: \n"
		"  %s [OPTION]... /path/to/old.ttf /path/to/new.ttf\n"
		"\n"
		"Options:\n"
		"  -renderSize=512\n"
		"             Size of rendered glyphs.\n"
		"  -accuracy=5\n"
		"             Consider glyphs different only if at least one continous\n"
		"             area exists of the given size (5px x 5px) or larger\n"
		"             which is ПОЛНОСТЬЮ different in both glyphs\n"
		"  -logKept   Log glyphs that are not changed.\n"
		"  -outputTarget=png:/output/pngs/prefix\n"
		"             Output each change to png file instead of stdout. File\n"
		"             name is appended to the value provided,\n"
		"             e.g. /output/pngs/prefixu002e.diff.png . If providing\n"
		"             a directory end the arument with a slash,\n"
		"             e.g. -outputTarget=png:/path/to/pngs/dir/ .\n"
		"  -outputSelect=diff\n"
		"             Changes to output, separated by comma. Possible values\n"
		"             are: diff, new, old .\n"
		"  -?, -h, -help, --help\n"
		"             Show this message.\n"
		"\n"
		"Version: %s\n",
		argv[0], VERSION);
	return 1;
}
