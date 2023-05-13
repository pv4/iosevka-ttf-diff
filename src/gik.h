#ifndef _20221228_055431_gik_h
#define _20221228_055431_gik_h

// Glyph identity key
typedef struct {
	char val[64];
	unsigned len;
} gik_t;

extern void gik_copy (gik_t *to, gik_t *from);
extern int gik_eq (gik_t *to, gik_t *from);

#endif
