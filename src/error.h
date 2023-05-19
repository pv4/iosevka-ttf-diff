#ifndef h_20221228_055431_error
#define h_20221228_055431_error

extern int error_code;
enum {
	ERROR_NONE = 0,
	ERROR_EXIT
};

extern void error_printf(const char *fmt, ...);

#define goto_throw(label, code) \
	do { \
		error_code = code; \
		goto catch_##label; \
	} while (0)

#define goto_throw_verbose(label, code, fmt_args) \
	do { \
		(error_printf)fmt_args; \
		error_code = code; \
		goto catch_##label; \
	} while (0)

#define goto_if_rethrow(label) \
	do { \
		if (ERROR_NONE != error_code) goto catch_##label; \
	} while(0)

/*
#define error_clear() \
	do { error_code = ERROR_NONE; } while(0)
#define switch_error \
	switch (error_code)
*/
#endif
