#include "gik.h"
#include <string.h>

void gik_copy (gik_t *to, gik_t *from) {
	strcpy(to->val, from->val);
	to->len = from->len;
}

int gik_eq (gik_t *to, gik_t *from)
	{ return !strcmp(to->val, from->val); }
