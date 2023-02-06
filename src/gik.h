#ifndef _20221228_055431_gik_h
#define _20221228_055431_gik_h

// Glyph identity key
typedef struct {
	char val[64];
	unsigned len;
} Gik;

extern void gik_copy (Gik *to, Gik *from);
extern int gik_eq (Gik *to, Gik *from);

#endif
