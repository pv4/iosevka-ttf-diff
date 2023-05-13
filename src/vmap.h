#ifndef h_20221228_055431_vmap
#define h_20221228_055431_vmap

#include "main.h"

typedef struct {
	struct {
		hb_codepoint_t otag, ntag;
		uint32_t oval, nval;
	} map[1024];
	unsigned len;
} vmap_t;

extern void vmap_init (vmap_t *vm);
extern void vmap_term (vmap_t *vm);
extern void vmap_load_e (vmap_t *vm, const char *file);

extern void vmap_upgrade_tag (vmap_t *vm, hb_codepoint_t *ntag, hb_codepoint_t otag);
extern void vmap_upgrade_value (vmap_t *vm, hb_codepoint_t *ntag, uint32_t *nval, hb_codepoint_t otag, uint32_t oval);
extern void vmap_downgrade_tag (vmap_t *vm, hb_codepoint_t *otag, hb_codepoint_t ntag);
extern void vmap_downgrade_value (vmap_t *vm, hb_codepoint_t *otag, uint32_t *oval, hb_codepoint_t ntag, uint32_t nval);

#endif
