#ifndef h_20221228_055431_args
#define h_20221228_055431_args

typedef struct {
	int render_size, accuracy;
	int log_kept, out_diff, out_new, out_old;
	const char *out_png, *old_file, *new_file, *vmap_file;
} args_t;

int args_init (args_t *a, int argc, char *argv[]);

#endif
