#include "vmap.h"
#include "error.h"

void vmap_init (vmap_t *vm) { vm->len = 0; }
void vmap_term (vmap_t *vm) {}

//fixme: HbCodepoint or HbTag ?
static void parse_variant (char **pp, hb_codepoint_t *tag, uint32_t *val) {
	uint32_t v;
	char *p;
	
	p = *pp;
	*tag = hb_tag_from_string(p, 4);
	p += 4;
	
	if (*p != '=') {
		*val = 0;
		*pp = p;
		return;
	}
	
	for (v = 0, p++; ; p++)
		switch (*p) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			v = v*10 + *p-'0';
			break;
		default:
			*val = v;
			*pp = p;
			return;
		}
}

void vmap_load_e (vmap_t *vm, const char *file) {
	// "????[.???]:????[.???]\n\0" and also comments/empty lines
	static char buf[255/*19*/];
	//TODO: stdio.h not needed?
	FILE *f;
	char *p;
	hb_codepoint_t otag, ntag;
	uint32_t oval, nval;
	unsigned len;
	
	f = fopen(file, "r");
	if (!f)
		goto_throw_verbose(opening, ERROR_EXIT, (
			"ERROR opening variant map \"%s\"\n", file));
	
	len = 0;
	while (fgets(buf, sizeof buf, f)) {
		p = buf;
		if (*p == '\n' || *p == '#') continue;
		
		if (*p == ':') otag = HB_TAG_NONE, oval = 0;
		else parse_variant(&p, &otag, &oval);
		if (*p++ != ':')
			goto_throw_verbose(parsing, ERROR_EXIT, (
				"ERROR parsing variant map \"%s\": colon not found\n", file));
		
		if (*p == '\n') ntag = HB_TAG_NONE, nval = 0;
		else parse_variant(&p, &ntag, &nval);
		if (*p++ != '\n')
			goto_throw_verbose(parsing, ERROR_EXIT, (
				"ERROR parsing variant map \"%s\": extra data after mapping\n", file));
		
		vm->map[len].otag = otag;
		vm->map[len].oval = oval;
		vm->map[len].ntag = ntag;
		vm->map[len].nval = nval;
		len++;
	}
	vm->len = len;
	
	fclose(f);
	return;
	
catch_parsing:
	fclose(f);
catch_opening:
}

void vmap_upgrade_tag (vmap_t *vm, hb_codepoint_t *ntag, hb_codepoint_t otag) {
	unsigned i;
	
	for (i = 0; i != vm->len; i++) {
		if (vm->map[i].otag != otag) continue;
		
		*ntag = !vm->map[i].ntag ? HB_TAG_NONE : vm->map[i].ntag;
		return;
	}
	
	for (i = 0; i != vm->len; i++) {
		if (vm->map[i].ntag != otag) continue;
		
		*ntag = HB_TAG_NONE;
		return;
	}
	
	*ntag = otag;
}

void vmap_downgrade_tag (vmap_t *vm, hb_codepoint_t *otag, hb_codepoint_t ntag) {
	unsigned i;
	
	for (i = 0; i != vm->len; i++) {
		if (vm->map[i].ntag != ntag) continue;
		
		*otag = !vm->map[i].otag ? HB_TAG_NONE : vm->map[i].otag;
		return;
	}
	
	for (i = 0; i != vm->len; i++) {
		if (vm->map[i].otag != ntag) continue;
		
		*otag = HB_TAG_NONE;
		return;
	}
	
	*otag = ntag;
}

static unsigned vmap_ntag_first (vmap_t *vm, hb_codepoint_t ntag) {
	unsigned i;
	
	for (i = 0; i != vm->len; i++)
		if (vm->map[i].ntag == ntag)
			return i;
	
	return UINT_MAX;
}

static unsigned vmap_otag_first (vmap_t *vm, hb_codepoint_t otag) {
	unsigned i;
	
	for (i = 0; i != vm->len; i++)
		if (vm->map[i].otag == otag)
			return i;
	
	return UINT_MAX;
}

void vmap_downgrade_value (vmap_t *vm, hb_codepoint_t *otag, uint32_t *oval, hb_codepoint_t ntag, uint32_t nval) {
	unsigned first, i;
	
	first = vmap_ntag_first(vm, ntag);
	if (UINT_MAX == first) {
		if (UINT_MAX != vmap_otag_first(vm, ntag))
			goto none;
		
		*otag = ntag;
		*oval = nval;
		return;
	}
	
	// if .ntag != HB_TAG_NONE then (.ntag != 0) == (.oval != 0)
	if (!vm->map[first].nval)
		goto otag_nval;
	
	for (i = first; i != vm->len; i++) {
		if (vm->map[i].ntag != ntag || vm->map[i].nval != nval)
			continue;
		
		if (!vm->map[i].otag)
			goto none;
		
		*otag = vm->map[i].otag;
		*oval = vm->map[i].oval;
		return;
	}
	
	for (i = first; i != vm->len; i++) {
		if (vm->map[i].ntag != ntag || vm->map[i].oval != nval)
			continue;
		
		goto none;
	}
	
otag_nval:
	*otag = vm->map[first].otag;
	*oval = nval;
	return;
	
none:
	*otag = HB_TAG_NONE;
	*oval = 0;
}

void vmap_upgrade_value (vmap_t *vm, hb_codepoint_t *ntag, uint32_t *nval, hb_codepoint_t otag, uint32_t oval) {
	unsigned first, i;
	
	first = vmap_otag_first(vm, otag);
	if (UINT_MAX == first) {
		if (UINT_MAX != vmap_ntag_first(vm, otag))
			goto none;
		
		*ntag = otag;
		*nval = oval;
		return;
	}
	
	// if .ntag != HB_TAG_NONE then (.ntag != 0) == (.oval != 0)
	if (!vm->map[first].oval)
		goto ntag_oval;
	
	for (i = first; i != vm->len; i++) {
		if (vm->map[i].otag != otag || vm->map[i].oval != oval)
			continue;
		
		if (!vm->map[i].ntag)
			goto none;
		
		*ntag = vm->map[i].ntag;
		*nval = vm->map[i].nval;
		return;
	}
	
	for (i = first; i != vm->len; i++) {
		if (vm->map[i].otag != otag || vm->map[i].nval != oval)
			continue;
		
		goto none;
	}
	
ntag_oval:
	*ntag = vm->map[first].ntag;
	*nval = oval;
	return;
	
none:
	*ntag = HB_TAG_NONE;
	*nval = 0;
}
