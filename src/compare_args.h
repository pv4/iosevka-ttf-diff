/* src/compare_args.h */

#ifndef h_20221228_055431_compare_args
#define h_20221228_055431_compare_args

enum {
	COMPARE_ARGS_LOG_DELETED = 0x1,
	COMPARE_ARGS_LOG_ADDED = 0x2,
	COMPARE_ARGS_LOG_CHANGED = 0x4,
	COMPARE_ARGS_LOG_KEPT = 0x8
};

enum {
	COMPARE_ARGS_OUT_DELETED = 0x1,
	COMPARE_ARGS_OUT_ADDED = 0x2,
	COMPARE_ARGS_OUT_CHANGED = 0x4
};

typedef struct {
	int render_size, accuracy;
	int log, out;
	const char *vmap_file, *out_png, *old_file, *new_file;
} compare_args_t;

int compare_args_init (compare_args_t *a, int argc, char *argv[]);

#endif
