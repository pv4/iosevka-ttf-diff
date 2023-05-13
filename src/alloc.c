#include "alloc.h"
#include <stdlib.h>

unsigned char *alloc_alloc_puchar (unsigned size)
	{ return (unsigned char *)malloc(sizeof(unsigned char) * size); }
void alloc_free_puchar (unsigned char *p) { free(p); }
