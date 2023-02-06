#include "gik.h"
#include <string.h>

void gik_copy (Gik *to, Gik *from) {
	strcpy(to->val, from->val);
	to->len = from->len;
}

int gik_eq (Gik *to, Gik *from)
	{ return !strcmp(to->val, from->val); }
