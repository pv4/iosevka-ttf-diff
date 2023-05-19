#ifndef h_20221228_055431_args
#define h_20221228_055431_args

enum {
	ARGS_LOG_DELETED = 0x1,
	ARGS_LOG_ADDED = 0x2,
	ARGS_LOG_CHANGED = 0x4,
	ARGS_LOG_KEPT = 0x8
};

enum {
	ARGS_OUT_DELETED = 0x1,
	ARGS_OUT_ADDED = 0x2,
	ARGS_OUT_CHANGED = 0x4
};

typedef struct {
	int render_size, accuracy;
	int log, out;
	const char *vmap_file, *out_png, *old_file, *new_file;
} args_t;

int args_init (args_t *a, int argc, char *argv[]);

#endif
