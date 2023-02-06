#include "alloc.h"
#include <stdlib.h>

unsigned char *Alloc_allocPuchar (unsigned size)
	{ return (unsigned char *)malloc(sizeof(unsigned char) * size); }
void Alloc_freePuchar (unsigned char *p) { free(p); }
