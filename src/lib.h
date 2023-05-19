#ifndef h_20221228_055431_lib
#define h_20221228_055431_lib

#include <string.h>

static inline int max (int a, int b) { return a > b ? a : b; }
static inline int min (int a, int b) { return a < b ? a : b; }

static inline const char *basename (const char *path) {
	const char *slash = strchr(path, '/');
	return !slash ? path : slash+1;
}

static inline int isdigit_c (char c) {
	switch (c) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return 1;
	default:
		return 0;
	}
}


#endif
